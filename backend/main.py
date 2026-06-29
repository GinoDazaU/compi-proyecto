"""API REST del compilador C++ → x86-64.

Expone el binario del compilador a la app web:

  GET  /api/health   → estado del servicio.
  POST /api/run      → compila, ensambla con g++ y ejecuta el binario.

Al arrancar, el servidor compila el compilador (build.py) y lo deja en
compiler/build/compiler. Si falla, igual arranca y /api/health lo reporta.
"""

import json
import logging
import os
import subprocess
import tempfile
import time
from contextlib import asynccontextmanager
from pathlib import Path

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
logger = logging.getLogger("compiler-api")

PROJECT_ROOT = Path(__file__).resolve().parent.parent
COMPILER_DIR = PROJECT_ROOT / "compiler"
COMPILER_BIN = COMPILER_DIR / "build" / "compiler"

COMPILE_TIMEOUT = 10           # seconds per compiler or g++ invocation
RUN_TIMEOUT = 15               # seconds for user binary execution
BUILD_TIMEOUT = 120            # seconds to build compiler on startup
OUTPUT_LIMIT = 64 * 1024       # stdout/stderr limit, in bytes


# ─── Arranque ─────────────────────────────────────────────────────────────────

def _build_compiler() -> None:
    """Compila el compilador con build.py; deja el binario en compiler/build/."""
    try:
        result = subprocess.run(["python3", "build.py", "build"], cwd=COMPILER_DIR,
                                capture_output=True, text=True, timeout=BUILD_TIMEOUT)
    except Exception as e:
        logger.warning("No se pudo compilar el compilador: %s", e)
        return
    if result.returncode == 0:
        logger.info("Compilador listo en %s", COMPILER_BIN)
    else:
        logger.warning("La compilación del compilador falló:\n%s", result.stderr.strip())


@asynccontextmanager
async def lifespan(app: FastAPI):
    _build_compiler()
    yield


app = FastAPI(title="Compilador C++ API", version="1.0.0", lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


# ─── Modelos ──────────────────────────────────────────────────────────────────

class SourceRequest(BaseModel):
    code: str
    optimize: bool = False


class ErrorInfo(BaseModel):
    type: str                  # lexical | syntax | semantic | server
    line: int = 0
    col: int = 0
    message: str


class RunResult(BaseModel):
    stdout: str
    stderr: str
    exit_code: int
    timed_out: bool


class Metrics(BaseModel):
    compile_ms: float | None = None         # fuente → assembly (compilador propio)
    assemble_ms: float | None = None        # assembly → binario (g++)
    exec_ms: float | None = None            # ejecución del binario
    binary_size_bytes: int | None = None


class CompileResponse(BaseModel):
    success: bool
    tokens: list | None = None
    ast: dict | None = None
    asm: str | None = None                  # codegen x86-64, solo si compiló
    error: ErrorInfo | None = None          # presente solo si success es False
    run: RunResult | None = None            # solo en /api/run, si llegó a ejecutar
    metrics: Metrics | None = None          # tiempos y tamaño medidos por fase


# ─── Helpers ──────────────────────────────────────────────────────────────────

def _server_error(message: str) -> CompileResponse:
    """Respuesta para fallos del lado del servidor (no del código del usuario)."""
    return CompileResponse(success=False, error=ErrorInfo(type="server", message=message))


def _clip(text: str) -> str:
    """Clips the program output to avoid flooding the response."""
    if len(text) > OUTPUT_LIMIT:
        return text[:OUTPUT_LIMIT] + "\n... (output truncated)"
    return text


def _timed(fn):
    """Ejecuta fn() y devuelve (resultado, milisegundos transcurridos)."""
    t0 = time.perf_counter()
    result = fn()
    return result, round((time.perf_counter() - t0) * 1000, 2)


def _run_compiler(mode: str, src: str, optimize: bool) -> subprocess.CompletedProcess:
    """Invoca el binario del compilador en el modo dado (--json, --asm, ...)."""
    argv = [str(COMPILER_BIN), mode]
    if optimize:
        argv.append("--opt")
    argv.append(src)
    return subprocess.run(argv, capture_output=True, text=True, timeout=COMPILE_TIMEOUT)


def _compile(code: str, optimize: bool) -> CompileResponse:
    """Returns tokens and AST (--json) and, if compilation succeeds, the assembly (--asm)."""
    if not COMPILER_BIN.exists():
        return _server_error("Compiler is not available; startup build failed (check server logs).")

    with tempfile.NamedTemporaryFile(mode="w", suffix=".cpp", delete=False) as f:
        f.write(code)
        src = f.name

    try:
        # Tokens and AST; in --json mode the compiler reports its errors as JSON.
        json_result = _run_compiler("--json", src, optimize)
        if not json_result.stdout.strip():
            return _server_error(json_result.stderr.strip() or "Compiler produced no output.")
        resp = CompileResponse(**json.loads(json_result.stdout))

        # Assembly, only if the code is valid.
        if resp.success:
            asm_result, compile_ms = _timed(lambda: _run_compiler("--asm", src, optimize))
            if asm_result.returncode == 0:
                resp.asm = asm_result.stdout
                resp.metrics = Metrics(compile_ms=compile_ms)
        return resp

    except subprocess.TimeoutExpired:
        return _server_error("Compilation exceeded the time limit.")
    except json.JSONDecodeError as e:
        return _server_error(f"Invalid response from compiler: {e}")
    finally:
        os.unlink(src)


def _assemble_and_run(asm: str) -> tuple[RunResult, Metrics]:
    """Ensambla el .s con g++ (-no-pie), ejecuta el binario y mide ambas fases."""
    metrics = Metrics()
    with tempfile.TemporaryDirectory() as tmp:
        asm_path = os.path.join(tmp, "program.s")
        exe_path = os.path.join(tmp, "program")
        with open(asm_path, "w") as f:
            f.write(asm)

        link, metrics.assemble_ms = _timed(lambda: subprocess.run(
            ["g++", "-no-pie", "-o", exe_path, asm_path],
            capture_output=True, text=True, timeout=COMPILE_TIMEOUT,
        ))
        if link.returncode != 0:
            return RunResult(stdout="", stderr=_clip(link.stderr),
                             exit_code=link.returncode, timed_out=False), metrics

        metrics.binary_size_bytes = os.path.getsize(exe_path)
        try:
            proc, metrics.exec_ms = _timed(lambda: subprocess.run(
                [exe_path], capture_output=True, text=True, timeout=RUN_TIMEOUT))
            return RunResult(stdout=_clip(proc.stdout), stderr=_clip(proc.stderr),
                             exit_code=proc.returncode, timed_out=False), metrics
        except subprocess.TimeoutExpired as e:
            partial = e.stdout.decode() if isinstance(e.stdout, bytes) else (e.stdout or "")
            return RunResult(stdout=_clip(partial),
                             stderr=f"Program exceeded the time limit of {RUN_TIMEOUT}s.",
                             exit_code=-1, timed_out=True), metrics


# ─── Endpoints ────────────────────────────────────────────────────────────────

@app.get("/api/health")
def health():
    """Estado del servicio y si el compilador está disponible."""
    return {"status": "ok", "compiler_ready": COMPILER_BIN.exists()}


@app.post("/api/run", response_model=CompileResponse)
def run_code(req: SourceRequest):
    """Compiles and, if there are no errors, assembles and runs. Returns the output."""
    resp = _compile(req.code, req.optimize)
    if resp.success and resp.asm:
        resp.run, run_metrics = _assemble_and_run(resp.asm)
        # _compile already set compile_ms; we add assemble, execution and size.
        resp.metrics.assemble_ms = run_metrics.assemble_ms
        resp.metrics.exec_ms = run_metrics.exec_ms
        resp.metrics.binary_size_bytes = run_metrics.binary_size_bytes
    return resp


# ─── Frontend estático ────────────────────────────────────────────────────────

FRONTEND_DIST = PROJECT_ROOT / "frontend" / "dist"
if FRONTEND_DIST.exists():
    app.mount("/", StaticFiles(directory=FRONTEND_DIST, html=True), name="static")

# ─── Entry point ──────────────────────────────────────────────────────────────

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)
