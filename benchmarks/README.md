# Benchmarks

Comparación del compilador propio contra GCC, Clang, Rust y Go sobre los mismos
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
│       ├── <benchmark>.rs       # Rust             → rustc      (opcional)
│       ├── <benchmark>.go       # Go               → go build   (opcional)
│       └── expected.txt         # salida correcta, idéntica en todas las versiones
└── results/             # salidas generadas (no editar a mano)
    ├── results.csv      # métricas crudas
    └── *.png            # gráficos
```

Convención: nombre de carpeta = nombre base de los archivos. `run.py` descubre
todo por patrón. Si falta `.rs`/`.go`, ese lenguaje se salta para ese benchmark.

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

## Metodología

- Mismo algoritmo en cada lenguaje; `expected.txt` valida que todos coincidan
  antes de medir.
- Programas CPU-bound: una sola impresión al final (mide cómputo, no I/O).
- GCC y Clang se corren en `-O0` (comparación justa) y `-O2` (muestra la brecha).
- Ejecución: N repeticiones, se reporta la mediana. Binario estático.

## Uso

```bash
python3 run.py            # corre todos los benchmarks
python3 run.py fib sort   # solo algunos
```

## Pendiente

- [ ] Implementar `run.py`.
- [ ] Escribir el algoritmo de cada benchmark en sus 4 lenguajes + `expected.txt`.
