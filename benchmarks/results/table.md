# Resultados de benchmarks

## Resumen

Tamaño = media; tiempos = media geométrica del ratio vs `g++ -O2`.

| toolchain | tamaño medio (bytes) | exec (×) | compile (×) |
|---|---|---|---|
| mio -O0 | 16478 | 3.51 | 0.64 |
| g++ -O0 | 16026 | 1.73 | 0.90 |
| clang++ -O0 | 16058 | 1.96 | 0.87 |
| rustc debug | 4346954 | 3.47 | 0.99 |
| mio -opt | 16478 | 3.55 | 0.65 |
| g++ -O2 | 16013 | 1.00 | 1.00 |
| clang++ -O2 | 16058 | 0.78 | 0.92 |
| rustc release | 4341426 | 0.83 | 1.01 |
| go build | 1896463 | 1.09 | 0.78 |

## Tiempo de compilación (ms)

### sin optimización

| benchmark | mio -O0 | g++ -O0 | clang++ -O0 | rustc debug |
|---|---|---|---|---|
| collatz | 147.6 | 202.9 | 194.9 | 237.3 |
| fib | 153.2 | 211.3 | 199.0 | 215.8 |
| mandelbrot | 163.9 | 213.7 | 222.4 | 246.5 |
| quicksort | 131.2 | 252.7 | 217.8 | 258.8 |
| series_pi | 145.1 | 207.1 | 198.7 | 203.8 |
| sieve | 194.4 | 295.6 | 203.0 | 247.5 |
| sort | 145.8 | 149.2 | 229.0 | 253.0 |

### con optimización

| benchmark | mio -opt | g++ -O2 | clang++ -O2 | rustc release | go build |
|---|---|---|---|---|---|
| collatz | 149.7 | 184.8 | 226.5 | 221.1 | 171.1 |
| fib | 161.8 | 330.0 | 205.8 | 206.1 | 170.7 |
| mandelbrot | 200.6 | 246.9 | 203.3 | 187.6 | 150.6 |
| quicksort | 164.6 | 264.4 | 234.9 | 227.0 | 176.2 |
| series_pi | 124.9 | 225.1 | 259.5 | 309.5 | 198.6 |
| sieve | 192.6 | 207.8 | 197.2 | 247.5 | 199.6 |
| sort | 119.7 | 244.6 | 225.0 | 323.1 | 265.4 |

## Tamaño del binario (bytes)

### sin optimización

| benchmark | mio -O0 | g++ -O0 | clang++ -O0 | rustc debug |
|---|---|---|---|---|
| collatz | 16352 | 15968 | 16016 | 4341672 |
| fib | 16200 | 15992 | 16040 | 4339312 |
| mandelbrot | 16664 | 15968 | 16016 | 4340784 |
| quicksort | 16632 | 16168 | 16160 | 4351352 |
| series_pi | 16344 | 15968 | 16016 | 4365352 |
| sieve | 16608 | 16120 | 16112 | 4346160 |
| sort | 16552 | 16000 | 16048 | 4344048 |

### con optimización

| benchmark | mio -opt | g++ -O2 | clang++ -O2 | rustc release | go build |
|---|---|---|---|---|---|
| collatz | 16352 | 15976 | 16016 | 4337456 | 1893721 |
| fib | 16200 | 16000 | 16040 | 4337520 | 1893730 |
| mandelbrot | 16664 | 15976 | 16016 | 4337720 | 1894259 |
| quicksort | 16632 | 16104 | 16160 | 4338552 | 1894392 |
| series_pi | 16344 | 15976 | 16016 | 4362848 | 1910950 |
| sieve | 16608 | 16056 | 16112 | 4337896 | 1894161 |
| sort | 16552 | 16008 | 16048 | 4337992 | 1894033 |

## Tiempo de ejecución (ms)

### sin optimización

| benchmark | mio -O0 | g++ -O0 | clang++ -O0 | rustc debug |
|---|---|---|---|---|
| collatz | 1625.2 | 619.0 | 1477.6 | 1569.9 |
| fib | 1307.8 | 1195.0 | 833.6 | 1262.0 |
| mandelbrot | 495.3 | 189.8 | 215.0 | 172.8 |
| quicksort | 275.5 | 191.6 | 158.5 | 1026.6 |
| series_pi | 172.1 | 36.8 | 50.5 | 70.8 |
| sieve | 32.0 | 25.7 | 23.5 | 38.8 |
| sort | 315.3 | 141.0 | 169.9 | 473.7 |

### con optimización

| benchmark | mio -opt | g++ -O2 | clang++ -O2 | rustc release | go build |
|---|---|---|---|---|---|
| collatz | 1598.3 | 348.2 | 229.0 | 236.8 | 384.0 |
| fib | 1326.1 | 259.6 | 439.4 | 480.6 | 740.1 |
| mandelbrot | 489.4 | 87.2 | 87.0 | 88.7 | 90.1 |
| quicksort | 259.6 | 105.0 | 95.8 | 106.0 | 113.5 |
| series_pi | 171.0 | 18.2 | 18.8 | 23.5 | 19.8 |
| sieve | 37.5 | 20.0 | 16.7 | 15.0 | 19.4 |
| sort | 311.8 | 251.8 | 48.8 | 56.4 | 124.1 |
