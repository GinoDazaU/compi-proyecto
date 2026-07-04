# Resultados de benchmarks

## Resumen

Tamaño = media; tiempos = media geométrica del ratio vs `g++ -O2`.

| toolchain | tamaño medio (bytes) | exec (×) | compile (×) |
|---|---|---|---|
| mio -O0 | 16550 | 6.26 | 0.73 |
| g++ -O0 | 16001 | 3.19 | 0.88 |
| clang++ -O0 | 15933 | 3.65 | 1.09 |
| mio -opt | 16533 | 5.74 | 0.77 |
| g++ -O2 | 15981 | 1.00 | 1.00 |
| clang++ -O2 | 15933 | 0.82 | 1.22 |

## Tiempo de compilación (ms)

### sin optimización

| benchmark | mio -O0 | g++ -O0 | clang++ -O0 |
|---|---|---|---|
| collatz | 117.9 | 102.0 | 130.5 |
| fib | 75.3 | 107.4 | 132.3 |
| mandelbrot | 89.9 | 105.8 | 122.9 |
| opt_heavy | 78.4 | 110.4 | 128.6 |
| quicksort | 86.0 | 98.1 | 122.2 |
| series_pi | 80.2 | 93.4 | 120.8 |
| sieve | 75.9 | 97.4 | 114.0 |
| sort | 76.5 | 99.6 | 138.5 |

### con optimización

| benchmark | mio -opt | g++ -O2 | clang++ -O2 |
|---|---|---|---|
| collatz | 133.9 | 115.7 | 132.8 |
| fib | 92.8 | 143.3 | 138.2 |
| mandelbrot | 81.6 | 113.2 | 139.5 |
| opt_heavy | 78.2 | 104.6 | 135.4 |
| quicksort | 85.2 | 121.5 | 141.1 |
| series_pi | 83.4 | 99.4 | 127.4 |
| sieve | 77.2 | 116.0 | 129.6 |
| sort | 89.7 | 119.2 | 194.7 |

## Tamaño del binario (bytes)

### sin optimización

| benchmark | mio -O0 | g++ -O0 | clang++ -O0 |
|---|---|---|---|
| collatz | 16432 | 15944 | 15896 |
| fib | 16288 | 15968 | 15920 |
| mandelbrot | 16752 | 15944 | 15896 |
| opt_heavy | 16432 | 15944 | 15896 |
| quicksort | 16728 | 16168 | 16040 |
| series_pi | 16432 | 15944 | 15896 |
| sieve | 16704 | 16120 | 15992 |
| sort | 16632 | 15976 | 15928 |

### con optimización

| benchmark | mio -opt | g++ -O2 | clang++ -O2 |
|---|---|---|---|
| collatz | 16432 | 15944 | 15896 |
| fib | 16288 | 15968 | 15920 |
| mandelbrot | 16752 | 15944 | 15896 |
| opt_heavy | 16296 | 15944 | 15896 |
| quicksort | 16728 | 16088 | 16040 |
| series_pi | 16432 | 15944 | 15896 |
| sieve | 16704 | 16040 | 15992 |
| sort | 16632 | 15976 | 15928 |

## Tiempo de ejecución (ms)

### sin optimización

| benchmark | mio -O0 | g++ -O0 | clang++ -O0 |
|---|---|---|---|
| collatz | 898.6 | 339.7 | 592.2 |
| fib | 642.9 | 508.7 | 482.5 |
| mandelbrot | 430.7 | 145.4 | 154.6 |
| opt_heavy | 371.9 | 154.2 | 220.3 |
| quicksort | 143.4 | 101.3 | 97.0 |
| series_pi | 110.6 | 31.0 | 41.8 |
| sieve | 15.2 | 14.2 | 13.0 |
| sort | 148.3 | 87.1 | 86.7 |

### con optimización

| benchmark | mio -opt | g++ -O2 | clang++ -O2 |
|---|---|---|---|
| collatz | 778.2 | 134.8 | 104.2 |
| fib | 659.9 | 136.8 | 252.5 |
| mandelbrot | 433.3 | 54.1 | 53.8 |
| opt_heavy | 221.5 | 1.3 | 1.3 |
| quicksort | 143.7 | 53.1 | 53.3 |
| series_pi | 111.7 | 12.9 | 13.1 |
| sieve | 14.2 | 9.0 | 9.3 |
| sort | 147.5 | 175.1 | 24.5 |
