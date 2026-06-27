#!/usr/bin/env python3
"""Genera las visualizaciones de los benchmarks a partir de results/results.csv.

Para no saturar con todas las toolchains juntas, cada métrica se separa en dos
grupos: "sin optimización" (comparación justa, todos en -O0) y "con
optimización" (comparación real). Produce en results/:

    - table.md                tablas en Markdown (2 por métrica), listas para el reporte
    - <metrica>_unopt.png      gráfico del grupo sin optimización
    - <metrica>_opt.png        gráfico del grupo con optimización

La tabla es Python puro; los PNG requieren matplotlib (si falta, se omiten).

Uso:
    python3 plot.py
"""

import os
import csv
import sys

HERE    = os.path.dirname(os.path.abspath(__file__))
RESULTS = os.path.join(HERE, "results", "results.csv")
OUT_DIR = os.path.join(HERE, "results")

# Métricas: clave en data, título, formateador a texto y formato de etiqueta.
METRICS = [
    ("compile", "Tiempo de compilación (ms)", lambda v: "{:.1f}".format(v), "%.0f"),
    ("size",    "Tamaño del binario (bytes)", lambda v: "{:d}".format(int(v)), "%.0f"),
    ("exec",    "Tiempo de ejecución (ms)",   lambda v: "{:.1f}".format(v), "%.0f"),
]

# Grupos de toolchains. El orden define columnas y colores; solo se usan las
# que estén presentes en el CSV.
GROUPS = [
    ("sin optimización", "unopt", ["mio -O0",  "g++ -O0", "clang++ -O0", "rustc debug"]),
    ("con optimización", "opt",   ["mio -opt", "g++ -O2", "clang++ -O2", "rustc release", "go build"]),
]


def load():
    """Lee el CSV → (benchmarks, presentes, data[(bench,tool)] = métricas)."""
    benchmarks, present, data = [], set(), {}
    with open(RESULTS) as f:
        for row in csv.DictReader(f):
            bench = row["benchmark"]
            tool  = "{} {}".format(row["compilador"], row["flags"])
            if bench not in benchmarks:
                benchmarks.append(bench)
            present.add(tool)
            data[(bench, tool)] = {
                "compile": float(row["tiempo_compilacion_ms"]),
                "size":    float(row["tamano_binario_bytes"]),
                "exec":    float(row["tiempo_ejecucion_ms"]),
            }
    return benchmarks, present, data


def group_tools(present, tools):
    """Toolchains del grupo que sí están en el CSV, en el orden definido."""
    return [t for t in tools if t in present]


# ─── Tabla Markdown ──────────────────────────────────────────────────────────

def write_table_md(benchmarks, present, data):
    lines = ["# Resultados de benchmarks", ""]
    for key, title, fmt, _ in METRICS:
        lines.append("## " + title)
        lines.append("")
        for gtitle, _suffix, tools in GROUPS:
            cols = group_tools(present, tools)
            if not cols:
                continue
            lines.append("### " + gtitle)
            lines.append("")
            lines.append("| benchmark | " + " | ".join(cols) + " |")
            lines.append("|" + "---|" * (len(cols) + 1))
            for b in benchmarks:
                cells = []
                for t in cols:
                    v = data.get((b, t), {}).get(key)
                    cells.append(fmt(v) if v is not None else "—")
                lines.append("| " + b + " | " + " | ".join(cells) + " |")
            lines.append("")

    out = os.path.join(OUT_DIR, "table.md")
    with open(out, "w") as f:
        f.write("\n".join(lines))
    print("escrito", os.path.relpath(out, HERE))


# ─── Gráficas (requieren matplotlib) ─────────────────────────────────────────

def grouped_bars(plt, benchmarks, cols, values, title, ylabel, fname, label_fmt):
    n = len(cols)
    width = 0.8 / n
    x = range(len(benchmarks))

    fig, ax = plt.subplots(figsize=(max(8, 1.6 * len(benchmarks)), 5))
    for i, tool in enumerate(cols):
        heights = [values.get((b, tool)) or 0 for b in benchmarks]
        offsets = [xi + (i - (n - 1) / 2) * width for xi in x]
        bars = ax.bar(offsets, heights, width, label=tool)
        ax.bar_label(bars, fmt=label_fmt, fontsize=6, padding=2, rotation=90)

    ax.set_title(title)
    ax.set_ylabel(ylabel)
    ax.set_xticks(list(x))
    ax.set_xticklabels(benchmarks, rotation=30, ha="right")
    ax.set_yscale("log")  # rango amplio entre benchmarks; log los hace comparables
    ax.legend(fontsize=8)
    ax.grid(axis="y", linestyle=":", alpha=0.5)
    fig.tight_layout()

    out = os.path.join(OUT_DIR, fname)
    fig.savefig(out, dpi=120)
    plt.close(fig)
    print("escrito", os.path.relpath(out, HERE))


def write_plots(benchmarks, present, data):
    try:
        import matplotlib
        matplotlib.use("Agg")  # backend headless: no necesita display
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib no instalado: se omiten los PNG (la tabla sí se generó).")
        print("  instálalo con:  pip install matplotlib")
        return

    for key, title, _fmt, label_fmt in METRICS:
        values = {k: v[key] for k, v in data.items()}
        for gtitle, suffix, tools in GROUPS:
            cols = group_tools(present, tools)
            if not cols:
                continue
            grouped_bars(plt, benchmarks, cols, values,
                         "{} — {}".format(title, gtitle),
                         "log", "{}_{}.png".format(key, suffix), label_fmt)


def main():
    if not os.path.isfile(RESULTS):
        sys.exit("No existe {}. Corre primero run.py.".format(RESULTS))

    benchmarks, present, data = load()
    if not benchmarks:
        sys.exit("results.csv no tiene datos. Corre run.py.")

    write_table_md(benchmarks, present, data)
    write_plots(benchmarks, present, data)


if __name__ == "__main__":
    main()
