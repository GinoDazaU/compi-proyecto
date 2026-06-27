#!/usr/bin/env python3
"""Genera los gráficos de los benchmarks a partir de results/results.csv.

Produce un PNG por métrica en results/:
    - compile_time.png   tiempo de compilación (ms)
    - binary_size.png    tamaño del binario (bytes, escala log)
    - exec_time.png      tiempo de ejecución (ms, escala log)
    - exec_relative.png  ejecución relativa al baseline (g++ -O2 = 1.0)

Uso:
    python3 plot.py

Requiere matplotlib:
    pip install matplotlib      (idealmente dentro de un venv)
"""

import os
import csv
import sys

try:
    import matplotlib
    matplotlib.use("Agg")  # backend headless: no necesita display
    import matplotlib.pyplot as plt
except ImportError:
    sys.exit("Falta matplotlib. Instálalo con:  pip install matplotlib")

HERE     = os.path.dirname(os.path.abspath(__file__))
RESULTS  = os.path.join(HERE, "results", "results.csv")
OUT_DIR  = os.path.join(HERE, "results")
BASELINE = "g++ -O2"   # referencia para la gráfica de ejecución relativa


def load():
    """Lee el CSV y devuelve (benchmarks, toolchains, data[(bench,tool)] = fila).
    Conserva el orden de aparición para que colores y leyenda sean estables."""
    benchmarks, toolchains, data = [], [], {}
    with open(RESULTS) as f:
        for row in csv.DictReader(f):
            bench = row["benchmark"]
            tool  = "{} {}".format(row["compilador"], row["flags"])
            if bench not in benchmarks:
                benchmarks.append(bench)
            if tool not in toolchains:
                toolchains.append(tool)
            data[(bench, tool)] = {
                "compile": float(row["tiempo_compilacion_ms"]),
                "size":    float(row["tamano_binario_bytes"]),
                "exec":    float(row["tiempo_ejecucion_ms"]),
            }
    return benchmarks, toolchains, data


def grouped_bars(benchmarks, toolchains, values, title, ylabel, fname, logy=False):
    """values[(bench,tool)] -> número (o None para barra ausente)."""
    n = len(toolchains)
    width = 0.8 / n
    x = range(len(benchmarks))

    fig, ax = plt.subplots(figsize=(max(8, 1.6 * len(benchmarks)), 5))
    for i, tool in enumerate(toolchains):
        heights = [values.get((b, tool)) or 0 for b in benchmarks]
        offsets = [xi + (i - (n - 1) / 2) * width for xi in x]
        ax.bar(offsets, heights, width, label=tool)

    ax.set_title(title)
    ax.set_ylabel(ylabel)
    ax.set_xticks(list(x))
    ax.set_xticklabels(benchmarks, rotation=30, ha="right")
    if logy:
        ax.set_yscale("log")
    ax.legend(fontsize=8)
    ax.grid(axis="y", linestyle=":", alpha=0.5)
    fig.tight_layout()

    out = os.path.join(OUT_DIR, fname)
    fig.savefig(out, dpi=120)
    plt.close(fig)
    print("escrito", os.path.relpath(out, HERE))


def main():
    if not os.path.isfile(RESULTS):
        sys.exit("No existe {}. Corre primero run.py.".format(RESULTS))

    benchmarks, toolchains, data = load()
    if not benchmarks:
        sys.exit("results.csv no tiene datos. Corre run.py.")

    compile_v = {k: v["compile"] for k, v in data.items()}
    size_v    = {k: v["size"]    for k, v in data.items()}
    exec_v    = {k: v["exec"]    for k, v in data.items()}

    grouped_bars(benchmarks, toolchains, compile_v,
                 "Tiempo de compilación", "ms", "compile_time.png")
    grouped_bars(benchmarks, toolchains, size_v,
                 "Tamaño del binario", "bytes (log)", "binary_size.png", logy=True)
    grouped_bars(benchmarks, toolchains, exec_v,
                 "Tiempo de ejecución", "ms (log)", "exec_time.png", logy=True)

    # Ejecución relativa al baseline (cuántas veces más lento que g++ -O2).
    rel = {}
    for b in benchmarks:
        base = data.get((b, BASELINE), {}).get("exec")
        if not base:
            continue
        for t in toolchains:
            e = data.get((b, t), {}).get("exec")
            if e:
                rel[(b, t)] = e / base
    if rel:
        grouped_bars(benchmarks, toolchains, rel,
                     "Ejecución relativa ({} = 1.0)".format(BASELINE),
                     "× más lento", "exec_relative.png")


if __name__ == "__main__":
    main()
