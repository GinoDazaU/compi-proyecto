# Resultados de benchmarks

## Resumen

Tamaño = media; tiempos = media geométrica del ratio vs `g++ -O2`.

| toolchain | tamaño medio (bytes) | exec (×) | compile (×) |
|---|---|---|---|
| mio -O0 | 16497 | 6.31 | 0.74 |
| g++ -O0 | 16019 | 3.11 | 0.89 |
| clang++ -O0 | 16053 | 3.46 | 0.91 |
| rustc debug | 4346182 | 6.20 | 1.04 |
| mio -opt | 16480 | 5.62 | 0.77 |
| g++ -O2 | 16009 | 1.00 | 1.00 |
| clang++ -O2 | 16053 | 0.82 | 1.00 |
| rustc release | 4340919 | 0.77 | 1.05 |

## Tiempo de compilación (ms)

### sin optimización

| benchmark | mio -O0 | g++ -O0 | clang++ -O0 | rustc debug |
|---|---|---|---|---|
| collatz | 249.3 | 229.4 | 198.5 | 239.3 |
| fib | 136.5 | 186.7 | 213.9 | 211.3 |
| mandelbrot | 140.1 | 250.0 | 218.1 | 282.4 |
| opt_heavy | 152.1 | 182.4 | 255.1 | 218.3 |
| quicksort | 222.3 | 230.1 | 208.0 | 237.7 |
| series_pi | 201.0 | 196.0 | 245.9 | 243.3 |
| sieve | 175.5 | 249.6 | 184.1 | 336.9 |
| sort | 150.3 | 165.0 | 182.9 | 200.5 |

### con optimización

| benchmark | mio -opt | g++ -O2 | clang++ -O2 | rustc release |
|---|---|---|---|---|
| collatz | 203.3 | 227.2 | 261.4 | 207.5 |
| fib | 164.2 | 227.6 | 214.7 | 212.1 |
| mandelbrot | 164.5 | 226.3 | 228.9 | 241.3 |
| opt_heavy | 162.3 | 218.3 | 265.4 | 226.3 |
| quicksort | 224.7 | 349.8 | 218.9 | 342.3 |
| series_pi | 199.2 | 221.2 | 242.8 | 275.3 |
| sieve | 189.0 | 248.3 | 245.9 | 279.5 |
| sort | 147.4 | 183.8 | 202.8 | 217.1 |

## Tamaño del binario (bytes)

### sin optimización

| benchmark | mio -O0 | g++ -O0 | clang++ -O0 | rustc debug |
|---|---|---|---|---|
| collatz | 16384 | 15968 | 16016 | 4341672 |
| fib | 16232 | 15992 | 16040 | 4339312 |
| mandelbrot | 16704 | 15968 | 16016 | 4340784 |
| opt_heavy | 16384 | 15968 | 16016 | 4340776 |
| quicksort | 16664 | 16168 | 16160 | 4351352 |
| series_pi | 16384 | 15968 | 16016 | 4365352 |
| sieve | 16640 | 16120 | 16112 | 4346160 |
| sort | 16584 | 16000 | 16048 | 4344048 |

### con optimización

| benchmark | mio -opt | g++ -O2 | clang++ -O2 | rustc release |
|---|---|---|---|---|
| collatz | 16384 | 15976 | 16016 | 4337456 |
| fib | 16232 | 16000 | 16040 | 4337520 |
| mandelbrot | 16704 | 15976 | 16016 | 4337720 |
| opt_heavy | 16248 | 15976 | 16016 | 4337368 |
| quicksort | 16664 | 16104 | 16160 | 4338552 |
| series_pi | 16384 | 15976 | 16016 | 4362848 |
| sieve | 16640 | 16056 | 16112 | 4337896 |
| sort | 16584 | 16008 | 16048 | 4337992 |

## Tiempo de ejecución (ms)

### sin optimización

| benchmark | mio -O0 | g++ -O0 | clang++ -O0 | rustc debug |
|---|---|---|---|---|
| collatz | 1550.9 | 560.6 | 1394.9 | 1474.3 |
| fib | 1248.9 | 1097.4 | 802.0 | 1126.1 |
| mandelbrot | 460.4 | 180.7 | 217.0 | 162.9 |
| opt_heavy | 538.7 | 242.3 | 176.1 | 576.0 |
| quicksort | 209.0 | 173.9 | 162.3 | 1045.4 |
| series_pi | 163.7 | 34.8 | 50.5 | 52.2 |
| sieve | 30.9 | 23.1 | 23.8 | 38.1 |
| sort | 284.2 | 132.3 | 141.0 | 383.4 |

### con optimización

| benchmark | mio -opt | g++ -O2 | clang++ -O2 | rustc release |
|---|---|---|---|---|
| collatz | 1479.2 | 315.0 | 206.2 | 231.9 |
| fib | 1244.9 | 235.2 | 387.0 | 400.8 |
| mandelbrot | 444.9 | 95.5 | 88.8 | 85.8 |
| opt_heavy | 234.5 | 0.9 | 1.4 | 1.2 |
| quicksort | 226.0 | 100.7 | 90.9 | 107.0 |
| series_pi | 177.7 | 20.3 | 17.6 | 17.2 |
| sieve | 26.2 | 18.0 | 16.1 | 13.4 |
| sort | 283.3 | 256.0 | 44.1 | 28.9 |
