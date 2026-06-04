import json
import os
import subprocess
import tempfile
from pathlib import Path

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

PROJECT_ROOT = Path(__file__).resolve().parent.parent
COMPILER_BIN = PROJECT_ROOT / "compiler" / "build" / "compiler"

app = FastAPI(title="Compilador C++ API", version="1.0.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


# ─── Models ───────────────────────────────────────────────────────────────────

class CompileRequest(BaseModel):
    code: str


class CompileResponse(BaseModel):
    success: bool
    tokens: list | None = None
    ast: dict | None = None
    error: dict | None = None


# ─── Endpoints ────────────────────────────────────────────────────────────────

@app.get("/api/health")
def health():
    return {"status": "ok", "compiler": str(COMPILER_BIN), "exists": COMPILER_BIN.exists()}


@app.post("/api/compile", response_model=CompileResponse)
def compile_code(req: CompileRequest):
    """
    recibe codigo fuente, lo compila usando el binario C++ con --json,
    y devuelve tokens + AST como JSON estructurado.
    """
    if not COMPILER_BIN.exists():
        return CompileResponse(
            success=False,
            error={"type": "server", "line": 0, "col": 0,
                   "message": f"Compilador no encontrado en {COMPILER_BIN}. Ejecuta 'python3 build.py' en compiler/."}
        )

    # Escribir código fuente a archivo temporal
    with tempfile.NamedTemporaryFile(mode="w", suffix=".cpp", delete=False) as f:
        f.write(req.code)
        tmp_path = f.name

    try:
        result = subprocess.run(
            [str(COMPILER_BIN), "--json", tmp_path],
            capture_output=True, text=True, timeout=10
        )

        # El compilador en modo --json siempre retorna JSON por stdout
        if result.stdout.strip():
            data = json.loads(result.stdout)
            return CompileResponse(**data)

        # Si no hay stdout, hubo un error inesperado
        return CompileResponse(
            success=False,
            error={"type": "server", "line": 0, "col": 0,
                   "message": result.stderr.strip() or "Error desconocido del compilador"}
        )

    except subprocess.TimeoutExpired:
        return CompileResponse(
            success=False,
            error={"type": "server", "line": 0, "col": 0,
                   "message": "Timeout: la compilación tomó más de 10 segundos"}
        )
    except json.JSONDecodeError as e:
        return CompileResponse(
            success=False,
            error={"type": "server", "line": 0, "col": 0,
                   "message": f"Error parseando respuesta del compilador: {e}"}
        )
    finally:
        os.unlink(tmp_path)


# ─── Entry point ──────────────────────────────────────────────────────────────

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)
