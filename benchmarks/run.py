#!/usr/bin/env python3
"""Runner de benchmarks.

Para cada caso en cases/ y cada toolchain disponible: compila (midiendo el
tiempo source→binario), valida la salida contra expected.txt y mide el tiempo
de ejecución. Vuelca todo a results/results.csv.

Uso:
    python3 run.py            # todos los casos
    python3 run.py fib sort   # solo algunos
"""

import os
import sys
import csv
import time
import shutil
import statistics
import subprocess
import tempfile

HERE      = os.path.dirname(os.path.abspath(__file__))
CASES_DIR = os.path.join(HERE, "cases")
RESULTS   = os.path.join(HERE, "results", "results.csv")
COMPILER  = os.path.join(HERE, "..", "compiler", "build", "compiler")

COMPILE_RUNS = 3   # repeticiones para la mediana de tiempo de compilación
EXEC_RUNS    = 5   # repeticiones para la mediana de tiempo de ejecución

CSV_HEADER = ["benchmark", "lenguaje", "compilador", "flags",
              "tiempo_compilacion_ms", "tamano_binario_bytes", "tiempo_ejecucion_ms"]


# ─── Definición de toolchains ────────────────────────────────────────────────
# Cada config produce los "pasos" de compilación: lista de (argv, stdout|None).
# 'tools' lista los ejecutables que deben existir; 'extra' es un check opcional.

def cpp(tool, flags):
    return {"lang": "c++", "tool": tool, "flags": flags, "ext": "cpp", "tools": [tool],
            "steps": lambda src, out: [([tool, flags, "-o", out, src], None)]}

def rust(flags, label):
    return {"lang": "rust", "tool": "rustc", "flags": label, "ext": "rs", "tools": ["rustc"],
            "steps": lambda src, out: [(["rustc"] + flags + ["-o", out, src], None)]}

MINE = {"lang": "propio", "tool": "mio", "flags": "-O0", "ext": "txt", "tools": ["g++"],
        "extra": lambda: os.path.isfile(COMPILER),
        # paso 1: compilador propio txt→.s ; paso 2: g++ ensambla y enlaza
        "steps": lambda src, out: [([COMPILER, "--asm", src], out + ".s"),
                                   (["g++", "-o", out, out + ".s"], None)]}

GO = {"lang": "go", "tool": "go", "flags": "build", "ext": "go", "tools": ["go"],
      "steps": lambda src, out: [(["go", "build", "-o", out, src], None)]}

CONFIGS = [
    MINE,
    cpp("g++",      "-O0"),
    cpp("g++",      "-O2"),
    cpp("clang++",  "-O0"),
    cpp("clang++",  "-O2"),
    rust([],        "debug"),
    rust(["-O"],    "release"),
    GO,
]


# ─── Helpers ─────────────────────────────────────────────────────────────────

def have(tool):
    return shutil.which(tool) is not None

def available(cfg):
    if not all(have(t) for t in cfg["tools"]):
        return False
    return cfg.get("extra", lambda: True)()

def run_steps(steps):
    """Ejecuta los pasos de compilación; lanza RuntimeError si alguno falla."""
    for argv, out_path in steps:
        out = open(out_path, "w") if out_path else subprocess.DEVNULL
        try:
            r = subprocess.run(argv, stdout=out, stderr=subprocess.PIPE, text=True)
        finally:
            if out_path:
                out.close()
        if r.returncode != 0:
            raise RuntimeError("fallo en `{}`:\n{}".format(" ".join(argv), r.stderr))

def median_ms(fn, repeats):
    times = []
    for _ in range(repeats):
        t0 = time.perf_counter()
        fn()
        times.append((time.perf_counter() - t0) * 1000)
    return round(statistics.median(times), 2)


# ─── Medición de un (caso, config) ───────────────────────────────────────────

def bench_one(case, cfg, outbin):
    src = os.path.join(CASES_DIR, case, "{}.{}".format(case, cfg["ext"]))
    if not os.path.isfile(src) or os.path.getsize(src) == 0:
        return None  # versión no escrita para este lenguaje

    expected = read_file(os.path.join(CASES_DIR, case, "expected.txt")).strip()
    steps    = cfg["steps"](src, outbin)

    # Compilación (mediana). La última corrida deja el binario final.
    try:
        comp_ms = median_ms(lambda: run_steps(steps), COMPILE_RUNS)
    except RuntimeError as e:
        print("  [SKIP] {:<10} compilación falló: {}".format(cfg["tool"], str(e).splitlines()[0]))
        return None

    # Validación de correctitud.
    got = subprocess.run([outbin], capture_output=True, text=True).stdout.strip()
    if got != expected:
        print("  [FAIL] {:<10} salida '{}' != esperado '{}'".format(cfg["tool"], got, expected))
        return None

    size_b  = os.path.getsize(outbin)
    exec_ms = median_ms(lambda: subprocess.run([outbin], stdout=subprocess.DEVNULL,
                                               stderr=subprocess.DEVNULL), EXEC_RUNS)

    label = "{} {}".format(cfg["tool"], cfg["flags"])
    print("  [OK  ] {:<14} comp={:>8.2f}ms  bin={:>8}B  exec={:>8.2f}ms"
          .format(label, comp_ms, size_b, exec_ms))
    return [case, cfg["lang"], cfg["tool"], cfg["flags"], comp_ms, size_b, exec_ms]


def read_file(path):
    with open(path) as f:
        return f.read()


# ─── Main ────────────────────────────────────────────────────────────────────

def main():
    wanted = set(sys.argv[1:])
    cases = sorted(d for d in os.listdir(CASES_DIR)
                   if os.path.isdir(os.path.join(CASES_DIR, d)))
    if wanted:
        cases = [c for c in cases if c in wanted]

    active = [c for c in CONFIGS if available(c)]
    skipped = [c["tool"] + " " + c["flags"] for c in CONFIGS if not available(c)]
    if skipped:
        print("Toolchains no disponibles (se saltan): {}\n".format(", ".join(skipped)))

    rows = []
    with tempfile.TemporaryDirectory() as tmp:
        for case in cases:
            print("● {}".format(case))
            for cfg in active:
                outbin = os.path.join(tmp, "{}_{}_{}".format(case, cfg["tool"], cfg["flags"]))
                row = bench_one(case, cfg, outbin)
                if row:
                    rows.append(row)
            print()

    os.makedirs(os.path.dirname(RESULTS), exist_ok=True)
    with open(RESULTS, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(CSV_HEADER)
        w.writerows(rows)
    print("{} filas escritas en {}".format(len(rows), os.path.relpath(RESULTS, HERE)))


if __name__ == "__main__":
    main()
