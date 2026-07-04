#!/usr/bin/env python3
"""Genera las visualizaciones de los benchmarks a partir de results/results.csv.

Produce dos niveles en results/:

  summary/   → para el reporte: 1 gráfico por métrica (todos los toolchains)
      size.png      tamaño medio del binario (bytes, log)
      exec.png      ejecución, media geométrica del ratio vs g++ -O2
      compile.png   compilación, media geométrica del ratio vs g++ -O2
  detail/    → apéndice/respaldo: por benchmark, separado en sin/con optimización

  table.md   → tablas en Markdown (resumen + detalle)

Razón del geomean: promediar tiempos crudos entre benchmarks heterogéneos
(collatz ~1200ms vs sieve ~16ms) lo dominan los lentos. La media geométrica del
ratio contra un baseline es la forma correcta de resumir una suite (estilo SPEC).

La tabla es Python puro; los PNG requieren matplotlib (si falta, se omiten).

Uso:
    python3 plot.py
"""

import os
import csv
import sys
import math

HERE        = os.path.dirname(os.path.abspath(__file__))
RESULTS     = os.path.join(HERE, "results", "results.csv")
OUT_DIR     = os.path.join(HERE, "results")
SUMMARY_DIR = os.path.join(OUT_DIR, "summary")
DETAIL_DIR  = os.path.join(OUT_DIR, "detail")
BASELINE    = "g++ -O2"   # referencia para los ratios de tiempo

# Métricas: clave, título, formateador de celda, formato de etiqueta de barra.
METRICS = [
    ("compile", "Tiempo de compilación (ms)", lambda v: "{:.1f}".format(v), "%.0f"),
    ("size",    "Tamaño del binario (bytes)", lambda v: "{:d}".format(int(v)), "%.0f"),
    ("exec",    "Tiempo de ejecución (ms)",   lambda v: "{:.1f}".format(v), "%.0f"),
]

# Grupos de toolchains para el detalle. El orden define columnas y colores; solo
# se usan las que estén presentes en el CSV.
GROUPS = [
    ("sin optimización", "unopt", ["mio -O0",  "g++ -O0", "clang++ -O0"]),
    ("con optimización", "opt",   ["mio -opt", "g++ -O2", "clang++ -O2"]),
]

# Color por "marca" del lenguaje (mismo color para opt y sin opt). Nuestro
# compilador en gris neutro.
COLORS = {
    "mio -O0":       "#444444",  # gris (nuestro)
    "mio -opt":      "#444444",
    "g++ -O0":       "#1F77B4",  # azul (GCC)
    "g++ -O2":       "#1F77B4",
    "clang++ -O0":   "#7030A0",  # morado (Clang/LLVM)
    "clang++ -O2":   "#7030A0",
}


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
    return [t for t in tools if t in present]


def ordered_tools(present):
    """Todos los toolchains presentes, en el orden de GROUPS."""
    seen, out = set(), []
    for _t, _s, tools in GROUPS:
        for t in tools:
            if t in present and t not in seen:
                seen.add(t)
                out.append(t)
    return out


def geomean(vals):
    return math.exp(sum(math.log(v) for v in vals) / len(vals)) if vals else None


def color_for(tool):
    """Color de marca del toolchain; gris neutro si es uno desconocido."""
    return COLORS.get(tool, "#999999")


# ─── Agregados de resumen ────────────────────────────────────────────────────

def size_means(benchmarks, tools, data):
    """Tamaño medio del binario por toolchain (es casi constante por benchmark)."""
    out = {}
    for t in tools:
        sizes = [data[(b, t)]["size"] for b in benchmarks if (b, t) in data]
        if sizes:
            out[t] = sum(sizes) / len(sizes)
    return out


def time_ratios(benchmarks, tools, data, key):
    """Media geométrica de (tiempo_tool / tiempo_baseline) por toolchain."""
    out = {}
    for t in tools:
        ratios = []
        for b in benchmarks:
            base = data.get((b, BASELINE), {}).get(key)
            val  = data.get((b, t), {}).get(key)
            if base and val:
                ratios.append(val / base)
        g = geomean(ratios)
        if g is not None:
            out[t] = g
    return out


# ─── Tabla Markdown ──────────────────────────────────────────────────────────

def write_table_md(benchmarks, present, data):
    tools = ordered_tools(present)
    lines = ["# Resultados de benchmarks", ""]

    # Resumen (una fila por toolchain).
    has_base = BASELINE in present
    smean = size_means(benchmarks, tools, data)
    erat  = time_ratios(benchmarks, tools, data, "exec")    if has_base else {}
    crat  = time_ratios(benchmarks, tools, data, "compile") if has_base else {}
    lines.append("## Resumen")
    lines.append("")
    lines.append("Tamaño = media; tiempos = media geométrica del ratio vs `{}`."
                 .format(BASELINE))
    lines.append("")
    lines.append("| toolchain | tamaño medio (bytes) | exec (×) | compile (×) |")
    lines.append("|---|---|---|---|")
    for t in tools:
        sz = "{:d}".format(int(smean[t])) if t in smean else "—"
        ex = "{:.2f}".format(erat[t]) if t in erat else "—"
        cm = "{:.2f}".format(crat[t]) if t in crat else "—"
        lines.append("| {} | {} | {} | {} |".format(t, sz, ex, cm))
    lines.append("")

    # Detalle por métrica y grupo.
    for key, title, fmt, _ in METRICS:
        lines.append("## " + title)
        lines.append("")
        for gtitle, _suffix, gtools in GROUPS:
            cols = group_tools(present, gtools)
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

def detail_bars(plt, benchmarks, cols, values, title, fname, label_fmt):
    n = len(cols)
    width = 0.8 / n
    x = range(len(benchmarks))

    fig, ax = plt.subplots(figsize=(max(8, 1.6 * len(benchmarks)), 5))
    for i, tool in enumerate(cols):
        heights = [values.get((b, tool)) or 0 for b in benchmarks]
        offsets = [xi + (i - (n - 1) / 2) * width for xi in x]
        bars = ax.bar(offsets, heights, width, label=tool, color=color_for(tool))
        ax.bar_label(bars, fmt=label_fmt, fontsize=6, padding=2, rotation=90)

    ax.set_title(title)
    ax.set_ylabel("log")
    ax.set_xticks(list(x))
    ax.set_xticklabels(benchmarks, rotation=30, ha="right")
    ax.set_yscale("log")
    ax.legend(fontsize=8)
    ax.grid(axis="y", linestyle=":", alpha=0.5)
    fig.tight_layout()
    fig.savefig(os.path.join(DETAIL_DIR, fname), dpi=120)
    plt.close(fig)
    print("escrito", os.path.relpath(os.path.join(DETAIL_DIR, fname), HERE))


def summary_bars(plt, tools, values, title, ylabel, fname, label_fmt,
                 colors, logy=False, baseline_line=False):
    fig, ax = plt.subplots(figsize=(max(6, 1.3 * len(tools)), 5))
    xs = range(len(tools))
    heights = [values.get(t, 0) for t in tools]
    bars = ax.bar(xs, heights, color=colors)
    ax.bar_label(bars, fmt=label_fmt, fontsize=8, padding=2)

    ax.set_title(title)
    ax.set_ylabel(ylabel)
    ax.set_xticks(list(xs))
    ax.set_xticklabels(tools, rotation=30, ha="right")
    if logy:
        ax.set_yscale("log")
    if baseline_line:
        ax.axhline(1.0, color="gray", linestyle="--", linewidth=1)
    ax.grid(axis="y", linestyle=":", alpha=0.5)
    fig.tight_layout()
    fig.savefig(os.path.join(SUMMARY_DIR, fname), dpi=120)
    plt.close(fig)
    print("escrito", os.path.relpath(os.path.join(SUMMARY_DIR, fname), HERE))


def write_plots(benchmarks, present, data):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib no instalado: se omiten los PNG (la tabla sí se generó).")
        print("  instálalo con:  pip install matplotlib")
        return

    os.makedirs(SUMMARY_DIR, exist_ok=True)
    os.makedirs(DETAIL_DIR, exist_ok=True)
    tools  = ordered_tools(present)
    colors = [color_for(t) for t in tools]  # color de marca por toolchain

    # ── Resumen ──
    summary_bars(plt, tools, size_means(benchmarks, tools, data),
                 "Tamaño del binario (media)", "bytes (log)",
                 "size.png", "%.0f", colors, logy=True)
    if BASELINE in present:
        summary_bars(plt, tools, time_ratios(benchmarks, tools, data, "exec"),
                     "Ejecución — geomean del ratio vs {}".format(BASELINE),
                     "× vs " + BASELINE, "exec.png", "%.2f", colors, baseline_line=True)
        summary_bars(plt, tools, time_ratios(benchmarks, tools, data, "compile"),
                     "Compilación — geomean del ratio vs {}".format(BASELINE),
                     "× vs " + BASELINE, "compile.png", "%.2f", colors, baseline_line=True)
    else:
        print("Aviso: falta el baseline '{}'; se omiten los resúmenes de tiempo."
              .format(BASELINE))

    # ── Detalle (por benchmark) ──
    for key, title, _fmt, label_fmt in METRICS:
        values = {k: v[key] for k, v in data.items()}
        for gtitle, suffix, gtools in GROUPS:
            cols = group_tools(present, gtools)
            if not cols:
                continue
            detail_bars(plt, benchmarks, cols, values,
                        "{} — {}".format(title, gtitle),
                        "{}_{}.png".format(key, suffix), label_fmt)


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
