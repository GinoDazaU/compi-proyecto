#!/usr/bin/env python3
import subprocess
import sys
import os

OUT_DIR = "build"
OUT_BIN = os.path.join(OUT_DIR, "compiler")

SOURCES = [
    "src/main.cpp",
    "src/lexer/token.cpp",
    "src/lexer/lexer.cpp",
]

FLAGS = ["-std=c++17", "-Wall", "-Wextra", "-I", "src"]


def build():
    os.makedirs(OUT_DIR, exist_ok=True)
    cmd = ["g++"] + FLAGS + ["-o", OUT_BIN] + SOURCES
    print("Compilando...")
    result = subprocess.run(cmd)
    if result.returncode != 0:
        sys.exit(result.returncode)
    print(f"Listo: {OUT_BIN}")


def run(args):
    if not args:
        print("Uso: python build.py run tests/input/<archivo>.txt")
        sys.exit(1)
    build()
    input_path = args[0]
    filename = os.path.basename(input_path).replace(".txt", "_tokens.txt")
    output_path = os.path.join("tests", "output", filename)
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w") as out:
        subprocess.run([f"./{OUT_BIN}", input_path], stdout=out)
    print(f"Output: {output_path}")


def clean():
    subprocess.run(["rm", "-rf", OUT_DIR])
    print("Limpio.")


COMMANDS = {
    "build": lambda: build(),
    "run":   lambda: run(sys.argv[2:]),
    "clean": lambda: clean(),
}

if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "build"
    if cmd not in COMMANDS:
        print(f"Comandos: {', '.join(COMMANDS)}")
        sys.exit(1)
    COMMANDS[cmd]()
