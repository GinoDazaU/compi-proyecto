# Benchmarks

Comparación del compilador propio contra GCC y Clang sobre los mismos
algoritmos. Mide las tres métricas que pide `docs/proyecto.txt`: tiempo de
compilación, tamaño del binario y velocidad de ejecución.

## Estructura

```
benchmarks/
├── README.md            # este archivo
├── run.py               # compila cada versión, mide y vuelca results/results.csv
├── plot.py              # lee results.csv y genera los gráficos en results/
├── cases/               # los programas (datos de entrada)
│   └── <benchmark>/
│       ├── <benchmark>.txt      # lenguaje propio  → compilador propio
│       ├── <benchmark>.cpp      # C++              → g++ y clang
│       └── expected.txt         # salida correcta, idéntica en todas las versiones
└── results/             # salidas generadas (no editar a mano)
    ├── results.csv      # métricas crudas
    └── *.png            # gráficos
```

Convención: nombre de carpeta = nombre base de los archivos. `run.py` descubre todo por patrón.

## Benchmarks

| Carpeta      | Qué ejercita                                   | Nivel |
|--------------|------------------------------------------------|-------|
| `fib`        | recursión, llamadas                            | mínimo |
| `sort`       | arrays, loops, comparaciones (bubble/insertion)| mínimo |
| `sieve`      | arrays, módulo, branch-heavy (criba)           | básico |
| `series_pi`  | float intensivo (serie de Leibniz)             | básico |
| `collatz`    | enteros, branches                              | básico |
| `quicksort`  | recursión + punteros + memoria dinámica        | reco. |
| `mandelbrot` | float anidado intensivo (escape-time)          | reco. |
| `opt_heavy`  | expresiones que el optimizador puede simplificar | reco. |

## Metodología

- Mismo algoritmo en cada lenguaje; `expected.txt` valida que todos coincidan
  antes de medir.
- Programas CPU-bound: una sola impresión al final (mide cómputo, no I/O).
- GCC y Clang se corren en `-O0` (comparación justa) y `-O2` (muestra la brecha).
- Ejecución: N repeticiones, se reporta la mediana. Binario estático.

## Uso

```bash
python3 run.py            # corre todos los benchmarks → results/results.csv
python3 run.py fib sort   # solo algunos
python3 plot.py           # genera los gráficos en results/ (requiere matplotlib)
```

> `run.py` reescribe `results.csv` completo en cada corrida. Para el CSV final
> corre **sin filtro**; el filtro por nombre es solo para iterar rápido.

## Entorno de medición

Para números confiables, correr en **Linux nativo**. En **WSL**, mover el repo al
filesystem de Linux (p.ej. `~/compi-proyecto`) y no dejarlo en `/mnt/c/...`: el
montaje de Windows es lento y añade mucho ruido, sobre todo a los tiempos de
compilación.
