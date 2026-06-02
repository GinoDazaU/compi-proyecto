#!/usr/bin/env python3
import subprocess
import sys
import os
import glob
from datetime import datetime

BUILD_DIR = "build"
BIN       = os.path.join(BUILD_DIR, "compiler")
TESTS_IN  = os.path.join("tests", "input")
TESTS_OUT = os.path.join("tests", "output")

SOURCES = [
    "src/main.cpp",
    "src/lexer/token.cpp",
    "src/lexer/lexer.cpp",
    "src/parser/ast_printer.cpp",
    "src/parser/ast_json_printer.cpp",
    "src/parser/parser.cpp",
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


def ensure_built():
    if not os.path.isfile(BIN):
        build()


# ─── Run one file ─────────────────────────────────────────────────────────────

def run(args):
    if not args:
        print("Uso: python build.py run <archivo>")
        sys.exit(1)
    ensure_built()
    subprocess.run([f"./{BIN}"] + args)


# ─── Test all ─────────────────────────────────────────────────────────────────

def run_one(input_path, out_dir):
    """Corre el compilador sobre input_path y escribe tokens.txt, ast.txt, ast.json y summary.txt en out_dir."""
    os.makedirs(out_dir, exist_ok=True)
    name = os.path.basename(input_path)
    errors = []

    # tokens
    r_tok = subprocess.run([f"./{BIN}", "--tokens", input_path],
                           capture_output=True, text=True)
    with open(os.path.join(out_dir, "tokens.txt"), "w") as f:
        f.write(r_tok.stdout)
    if r_tok.stderr:
        errors.append(r_tok.stderr.strip())

    # ast
    r_ast = subprocess.run([f"./{BIN}", "--ast", input_path],
                           capture_output=True, text=True)
    with open(os.path.join(out_dir, "ast.txt"), "w") as f:
        f.write(r_ast.stdout)
    if r_ast.stderr:
        errors.append(r_ast.stderr.strip())

    # json
    r_json = subprocess.run([f"./{BIN}", "--json", input_path],
                            capture_output=True, text=True)
    with open(os.path.join(out_dir, "ast.json"), "w") as f:
        f.write(r_json.stdout)
    if r_json.stderr:
        errors.append(r_json.stderr.strip())

    # summary
    ok = r_tok.returncode == 0 and r_ast.returncode == 0 and r_json.returncode == 0
    with open(os.path.join(out_dir, "summary.txt"), "w") as f:
        f.write(f"archivo : {name}\n")
        f.write(f"estado  : {'OK' if ok else 'ERROR'}\n")
        f.write(f"fecha   : {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        token_count = len([l for l in r_tok.stdout.splitlines() if l.strip()])
        f.write(f"tokens  : {token_count}\n")
        if errors:
            f.write("\nerrores:\n")
            for e in errors:
                f.write(f"  {e}\n")

    status = "OK " if ok else "ERR"
    print(f"  [{status}] {name} → {out_dir}/")
    return ok


def test():
    ensure_built()

    inputs = sorted(glob.glob(os.path.join(TESTS_IN, "*.txt")) +
                    glob.glob(os.path.join(TESTS_IN, "*.cpp")))

    if not inputs:
        print(f"No hay archivos en {TESTS_IN}/")
        sys.exit(1)

    print(f"Corriendo {len(inputs)} test(s)...\n")
    ok_count = 0
    for i, path in enumerate(inputs, start=1):
        out_dir = os.path.join(TESTS_OUT, f"output{i}")
        if run_one(path, out_dir):
            ok_count += 1

    print(f"\n{ok_count}/{len(inputs)} tests pasaron.")


# ─── Clean ────────────────────────────────────────────────────────────────────

def clean():
    subprocess.run(["rm", "-rf", BUILD_DIR])
    subprocess.run(["rm", "-rf", TESTS_OUT])
    print("Limpio.")


# ─── Entry point ──────────────────────────────────────────────────────────────

COMMANDS = {
    "build": lambda: build(),
    "run":   lambda: run(sys.argv[2:]),
    "test":  lambda: test(),
    "clean": lambda: clean(),
}

if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "build"
    if cmd not in COMMANDS:
        print(f"Comandos disponibles: {', '.join(COMMANDS)}")
        sys.exit(1)
    COMMANDS[cmd]()
