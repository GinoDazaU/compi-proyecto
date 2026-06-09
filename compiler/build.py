#!/usr/bin/env python3
import subprocess
import sys
import os
import glob

BUILD_DIR     = "build"
BIN           = os.path.join(BUILD_DIR, "compiler")
TESTS_IN        = os.path.join("tests", "input", "ok_input")
TESTS_ERR_IN    = os.path.join("tests", "input", "error_input")
TESTS_OUT       = os.path.join("tests", "output")
TESTS_OK_OUT    = os.path.join(TESTS_OUT, "ok_output")
TESTS_ERR_OUT   = os.path.join(TESTS_OUT, "error_output")
TESTS_SANDBOX   = os.path.join("tests", "sandbox")

SOURCES = [
    "src/main.cpp",
    "src/lexer/token.cpp",
    "src/lexer/lexer.cpp",
    "src/parser/ast_printer.cpp",
    "src/parser/ast_json_printer.cpp",
    "src/parser/parser.cpp",
    "src/semantic/sem_type.cpp",
    "src/semantic/type_checker.cpp",
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
    """Genera tokens.txt, ast.txt y ast.json en out_dir."""
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


def run_error_test(input_path, out_file):
    """Escribe el mensaje de error en out_file. Éxito = el compilador reportó error."""
    os.makedirs(os.path.dirname(out_file), exist_ok=True)
    name = os.path.basename(input_path)

    r = subprocess.run([f"./{BIN}", "--ast", input_path], capture_output=True, text=True)

    got_error = r.returncode != 0
    with open(out_file, "w") as f:
        f.write(r.stderr.strip() if r.stderr else "(sin mensaje)")

    print(f"  [{'OK ' if got_error else 'ERR'}] {name}")
    return got_error


def run_section(label, paths, run_fn, out_fn):
    """Corre una sección de tests y retorna (passed, total)."""
    if not paths:
        return 0, 0
    print(f"{label}")
    sec_passed = sum(run_fn(p, out_fn(p)) for p in paths)
    sec_total  = len(paths)
    print(f"  {sec_passed}/{sec_total}")
    return sec_passed, sec_total


def test():
    ensure_built()

    valid_inputs   = sorted(glob.glob(os.path.join(TESTS_IN,     "*.txt")) +
                            glob.glob(os.path.join(TESTS_IN,     "*.cpp")))
    error_inputs   = sorted(glob.glob(os.path.join(TESTS_ERR_IN, "*.txt")) +
                            glob.glob(os.path.join(TESTS_ERR_IN, "*.cpp")))
    sandbox_inputs = sorted(glob.glob(os.path.join(TESTS_SANDBOX, "*.txt")) +
                            glob.glob(os.path.join(TESTS_SANDBOX, "*.cpp")))

    total, passed = 0, 0

    def ok_out(p):
        return os.path.join(TESTS_OK_OUT, os.path.splitext(os.path.basename(p))[0])

    def err_out(p):
        return os.path.join(TESTS_ERR_OUT, os.path.splitext(os.path.basename(p))[0] + ".txt")

    def sb_out(p):
        return os.path.join(TESTS_SANDBOX, os.path.splitext(os.path.basename(p))[0])

    for label, paths, fn, out_fn in [
        ("ok_input/",    valid_inputs,   run_one,        ok_out),
        ("error_input/", error_inputs,   run_error_test, err_out),
        ("sandbox/",     sandbox_inputs, run_one,        sb_out),
    ]:
        p, t = run_section(label, paths, fn, out_fn)
        passed += p
        total  += t

    print(f"\n{'─' * 24}")
    if passed == total:
        print(f"{passed}/{total}  todo OK")
    else:
        print(f"{passed}/{total}  {total - passed} fallaron")


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
