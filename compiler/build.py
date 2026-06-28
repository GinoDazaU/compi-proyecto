#!/usr/bin/env python3
import subprocess
import sys
import os
import glob

BUILD_DIR     = "build"
BIN           = os.path.join(BUILD_DIR, "compiler")

TESTS_ANALYSIS = os.path.join("tests", "analysis")   # válidos: pasan el frontend
TESTS_ERRORS   = os.path.join("tests", "errors")     # deben ser rechazados
TESTS_SANDBOX  = os.path.join("tests", "sandbox")    # pruebas libres
TESTS_E2E      = os.path.join("tests", "e2e")        # compilan, ejecutan y comparan

# Artefactos generados (no versionados): viven bajo build/
TEST_OUT  = os.path.join(BUILD_DIR, "test-output")
E2E_BUILD = os.path.join(BUILD_DIR, "e2e")

SOURCES = [
    "src/main.cpp",
    "src/lexer/token.cpp",
    "src/lexer/lexer.cpp",
    "src/parser/ast_json_printer.cpp",
    "src/parser/parser.cpp",
    "src/semantic/sem_type.cpp",
    "src/semantic/type_checker.cpp",
    "src/codegen/code_generator.cpp",
    "src/optimizer/optimizer.cpp",
    "src/optimizer/ast_walker.cpp",
    "src/optimizer/constant_folder.cpp",
    "src/optimizer/constant_propagator.cpp",
    "src/optimizer/algebraic_simplifier.cpp",
    "src/optimizer/dead_code_eliminator.cpp",
]

FLAGS = ["-std=c++17", "-Wall", "-Wextra", "-I", "src"]


# ─── Build ────────────────────────────────────────────────────────────────────

def build():
    os.makedirs(BUILD_DIR, exist_ok=True)
    cmd = ["g++"] + FLAGS + ["-o", BIN] + SOURCES
    print("Compilando...")
    r = subprocess.run(cmd)
    if r.returncode != 0:
        sys.exit(r.returncode)
    print(f"Listo: {BIN}")


def inputs_in(folder):
    return sorted(glob.glob(os.path.join(folder, "*.txt")) +
                  glob.glob(os.path.join(folder, "*.cpp")))


# ─── Run one file ─────────────────────────────────────────────────────────────

def run(args):
    if not args:
        print("Uso: python build.py run <archivo>")
        sys.exit(1)
    build()
    subprocess.run([f"./{BIN}"] + args)


# ─── Tests del frontend (analysis / errors / sandbox) ─────────────────────────

def run_analysis_one(input_path, out_dir):
    """Genera tokens.txt, ast.txt y ast.json. Éxito = el compilador no falla."""
    os.makedirs(out_dir, exist_ok=True)
    name = os.path.basename(input_path)

    r_tok  = subprocess.run([f"./{BIN}", "--tokens", input_path], capture_output=True, text=True)
    r_ast  = subprocess.run([f"./{BIN}", "--ast",    input_path], capture_output=True, text=True)
    r_json = subprocess.run([f"./{BIN}", "--json",   input_path], capture_output=True, text=True)

    with open(os.path.join(out_dir, "tokens.txt"), "w") as f: f.write(r_tok.stdout)
    with open(os.path.join(out_dir, "ast.txt"),    "w") as f: f.write(r_ast.stdout)
    with open(os.path.join(out_dir, "ast.json"),   "w") as f: f.write(r_json.stdout)

    ok = r_tok.returncode == 0 and r_ast.returncode == 0 and r_json.returncode == 0
    print(f"  [{'OK ' if ok else 'ERR'}] {name}")
    return ok


def run_error_one(input_path, out_file):
    """Escribe el mensaje de error. Éxito = el compilador reportó error."""
    os.makedirs(os.path.dirname(out_file), exist_ok=True)
    name = os.path.basename(input_path)

    r = subprocess.run([f"./{BIN}", "--ast", input_path], capture_output=True, text=True)

    got_error = r.returncode != 0
    with open(out_file, "w") as f:
        f.write(r.stderr.strip() if r.stderr else "(sin mensaje)")

    print(f"  [{'OK ' if got_error else 'ERR'}] {name}")
    return got_error


# ─── Tests end-to-end (e2e) ───────────────────────────────────────────────────
# Para cada tests/e2e/*.txt: genera el .s, lo ensambla con g++, ejecuta el
# binario y compara su stdout contra el .expected del mismo nombre.

def run_e2e_one(src, _out=None):
    name = os.path.basename(src)
    base = os.path.splitext(name)[0]
    expected_file = os.path.join(TESTS_E2E, base + ".expected")
    asm = os.path.join(E2E_BUILD, base + ".s")
    exe = os.path.join(E2E_BUILD, base)

    if not os.path.isfile(expected_file):
        print(f"  [ERR] {name}  (falta {base}.expected)")
        return False

    # 1. Generar assembly
    r = subprocess.run([f"./{BIN}", "--asm", src], capture_output=True, text=True)
    if r.returncode != 0:
        print(f"  [ERR] {name}  (el compilador falló)")
        return False
    with open(asm, "w") as f:
        f.write(r.stdout)

    # 2. Ensamblar con g++
    asm_r = subprocess.run(["g++", "-no-pie", "-o", exe, asm], capture_output=True, text=True)
    if asm_r.returncode != 0:
        print(f"  [ERR] {name}  (g++ no pudo ensamblar)")
        return False

    # 3. Ejecutar y comparar stdout
    out = subprocess.run([exe], capture_output=True, text=True).stdout
    with open(expected_file) as f:
        expected = f.read()

    ok = out.rstrip("\n") == expected.rstrip("\n")
    print(f"  [{'OK ' if ok else 'ERR'}] {name}")
    return ok


# ─── Orquestación de secciones ────────────────────────────────────────────────

def run_section(label, paths, run_fn, out_fn):
    if not paths:
        return 0, 0
    print(f"{label}")
    passed = sum(run_fn(p, out_fn(p)) for p in paths)
    total  = len(paths)
    print(f"  {passed}/{total}")
    return passed, total


def test():
    build()
    os.makedirs(E2E_BUILD, exist_ok=True)

    def analysis_out(p):
        return os.path.join(TEST_OUT, "analysis", os.path.splitext(os.path.basename(p))[0])

    def error_out(p):
        return os.path.join(TEST_OUT, "errors", os.path.splitext(os.path.basename(p))[0] + ".txt")

    def sandbox_out(p):
        return os.path.join(TEST_OUT, "sandbox", os.path.splitext(os.path.basename(p))[0])

    sections = [
        ("analysis/", inputs_in(TESTS_ANALYSIS), run_analysis_one, analysis_out),
        ("errors/",   inputs_in(TESTS_ERRORS),   run_error_one,    error_out),
        ("sandbox/",  inputs_in(TESTS_SANDBOX),  run_analysis_one, sandbox_out),
        ("e2e/",      inputs_in(TESTS_E2E),      run_e2e_one,      lambda p: None),
    ]

    passed, total = 0, 0
    for label, paths, fn, out_fn in sections:
        p, t = run_section(label, paths, fn, out_fn)
        passed += p
        total  += t

    print(f"\n{'─' * 24}")
    if passed == total:
        print(f"{passed}/{total}  todo OK")
    else:
        print(f"{passed}/{total}  {total - passed} fallaron")


# ─── Entry point ──────────────────────────────────────────────────────────────

COMMANDS = {"build": build, "test": test}

if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "build"
    if cmd == "run":
        run(sys.argv[2:])
    elif cmd in COMMANDS:
        COMMANDS[cmd]()
    else:
        print("Comandos disponibles: build, run, test")
        sys.exit(1)
