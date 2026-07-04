# Compilador de C++ → x86-64

**Curso de Compiladores — Proyecto final**

**Repositorio:** <https://github.com/GinoDazaU/compi-proyecto>

**Demo en vivo:** <https://compi-proyecto.up.railway.app>

**Integrantes:** Gino Jesus Daza Yalta - Fali Ferdinand Araoz Arana

---

## 1. Resumen

Este proyecto es un **compilador de un subconjunto de C++** que genera código
ensamblador **x86-64** (sintaxis AT&T), escrito desde cero en **C++17**. Cubre el
pipeline completo de compilación —análisis léxico, sintáctico y semántico,
optimización sobre el AST y generación de código— y produce un `.s` que se
ensambla y enlaza con `g++` para obtener un ejecutable nativo.

Como complemento (bonus del proyecto), incluye una **aplicación web** que expone el
compilador como API y permite editar código, visualizar los tokens y el AST, ver el
ensamblador generado y ejecutar el programa.

Este documento es una **guía de navegación** del proyecto: resume qué hace cada
parte y remite a la documentación detallada en `docs/`, donde está el desarrollo
técnico completo.

---

## 2. Lenguaje soportado

El compilador acepta un subconjunto de C++ con las siguientes características:

| Categoría | Soporta |
|---|---|
| **Tipos básicos** | `int`, `float`, `bool`, `char`, `string`, `void` |
| **Tipos de usuario** | `struct` con atributos (paso por puntero) |
| **Variables y scope** | scopes anidados, shadowing, `auto` (inferencia) |
| **Funciones** | parámetros, valor de retorno, recursión |
| **Control de flujo** | `if/else`, `while`, `for`, `break`, `continue`, `return` |
| **Operadores** | aritméticos, lógicos (con cortocircuito), relacionales, asignación, `++`/`--` |
| **Arrays** | estáticos, **multidimensionales**, con `init_list` |
| **Punteros** | aritmética de punteros, deref, address-of |
| **Memoria dinámica** | `new` / `delete` / `new T[n]` / `delete[]` |
| **Conversiones** | promoción e inferencia automática de tipos (`int`→`float`, etc.) |
| **Salida** | built-ins `print` / `println` (variádicos) |

La gramática formal (CFG) está en **`docs/grammar.md`** y las reglas de tipos,
compatibilidad y scope en **`docs/semantic_rules.md`**.

---

## 3. Arquitectura y pipeline

El compilador es una cadena de transformaciones: cada fase consume la salida de la
anterior. No hay representación intermedia (IR); se va del AST directo al assembly.

```
código fuente (.txt)
      │  Lexer                    texto → tokens
      ▼  Parser                   tokens → AST
      ▼  Semántico (TypeChecker)  verifica tipos y scopes
      ▼  Optimizador (--opt)      reescribe el AST (opcional)
      ▼  CodeGenerator            AST → ensamblador x86-64
   archivo .s
      ▼  g++                      ensambla y enlaza → ejecutable
```

**Lexer** — Convierte el texto en *tokens* (keywords, identificadores, literales,
operadores), descartando espacios y comentarios. Cada token lleva su tipo, lexema y
posición (línea/columna) para el reporte de errores.

**Parser + AST** — Parser de **descenso recursivo**: cada regla de la gramática es
una función. Construye el **Árbol de Sintaxis Abstracta** respetando la precedencia
de operadores. El AST es la estructura central sobre la que trabajan todas las fases
siguientes, recorrido con el **patrón Visitor**.

**Semántico (TypeChecker)** — Dos pasadas: la primera recolecta las firmas globales
(structs y funciones) para permitir referencias cruzadas; la segunda verifica tipos,
scopes, lvalues, retornos y compatibilidad. Se apoya en `SemType` (tipos resueltos y
comparables) y una `SymbolTable` genérica de scopes anidados.

**Optimizador** — Fase opcional (flag `--opt`) que reescribe el AST *in-place*. Es
una cadena de pases pequeños (ver §4).

**CodeGenerator** — Traduce el AST a ensamblador x86-64 siguiendo la convención de
llamada System V AMD64: prólogo/epílogo estándar, variables en offsets desde `%rbp`,
resultado de toda expresión en `%rax`/`%xmm0`, argumentos en registros.

Las tres fases de análisis son **fail-fast**: ante el primer error lanzan una
excepción con línea y columna (`LexError`, `ParseError`, `SemanticError`) y la
cadena se detiene.

> **Detalle:** cada fase produce una representación inspeccionable. Los flags
> `--tokens`, `--ast`, `--json` y `--asm` permiten detener el compilador y volcar
> esa etapa —lo que además alimenta a la app web.

La explicación técnica **fase por fase** está en **`docs/guide/`**:

- `docs/guide/pipeline.md` — cómo se conectan todas las fases (`main.cpp`).
- `docs/guide/1_lexer/` — token y lexer.
- `docs/guide/2_parser/` — AST, patrón Visitor y parser.
- `docs/guide/3_semantic/` — `SemType`, tabla de símbolos y TypeChecker.
- `docs/guide/5_codegen/` — generador de código.

Las notas de diseño de codegen (registros, instrucciones por tipo, structs, arrays,
control de flujo) están en **`docs/codegen.md`**, y las decisiones puntuales en
**`docs/decisiones/`**.

---

## 4. Optimizaciones

Con el flag `--opt`, el AST validado pasa por una cadena de pases, cada uno
haciendo **una** cosa; la potencia surge de encadenarlos (el orden importa):

1. **Constant folding** — pliega operaciones entre literales (`2 + 3` → `5`).
2. **Constant propagation** — sustituye variables constantes por su literal.
3. **Constant folding** (de nuevo) — pliega lo que la propagación dejó expuesto.
4. **Algebraic simplification** — identidades como `x + 0` → `x`, `x * 1` → `x`.
5. **Dead code elimination** — descarta ramas de condición constante y código
   inalcanzable tras `return`/`break`/`continue`.

Todos los pases son **conservadores**: ante la menor duda no optimizan (no pliegan
división por cero, no propagan variables cuya dirección se toma, etc.), para nunca
alterar el comportamiento del programa.

Detalle de cada pase en **`docs/guide/4_optimizer/`**.

---

## 5. Aplicación web (bonus)

La app conecta un **backend Python (FastAPI)** que expone el compilador como API
REST con un **frontend React/Vite**. Permite:

- Editar código del lenguaje en un editor con resaltado.
- Visualizar los **tokens** y el **AST** generados.
- Ver el **ensamblador x86-64** producido.
- **Ejecutar** el programa compilado y ver su salida.

Cubre los componentes que la rúbrica pide para el logro máximo del proyecto (editor,
visualización de AST, generación x86, ejecución y visualización de resultados).

Hay un **deploy en vivo** en <https://compi-proyecto.up.railway.app>. También se
puede levantar localmente con Docker:

```bash
docker build -t compi . && docker run -p 8000:8000 compi
# disponible en localhost:8000
```

---

## 6. Pruebas

El proyecto incluye una batería de pruebas organizada por propósito, ejecutable con
`build.py`:

| Suite | Qué valida | Cantidad |
|---|---|---|
| `tests/analysis/` | programas válidos que deben pasar el frontend | 14 |
| `tests/errors/` | programas que **deben** ser rechazados (léxico/sintáctico/semántico) | 19 |
| `tests/e2e/` | compilan, ejecutan y comparan `stdout` contra un `.expected` | 71 |

```bash
cd compiler
python3 build.py test        # corre toda la batería
```

Los tests end-to-end (`e2e`) son la validación más fuerte: verifican que el
ejecutable generado produce exactamente la salida esperada, cubriendo aritmética,
control de flujo, arrays (incl. multidimensionales), structs, punteros, memoria
dinámica, strings y cortocircuito lógico.

---

## 7. Benchmarks y Evaluación Comparativa

Evaluación cuantitativa del compilador `mio` frente a los toolchains comerciales **GCC** y **Clang/LLVM** sobre un conjunto de 8 programas de prueba en un entorno Linux nativo. Para un análisis detallado a bajo nivel y con el desglose completo por benchmark de cada métrica, ver el documento anexo **[benchmarks.md](benchmarks.md)**.

### 7.1 Metodología y Métricas
Se midieron tres ejes fundamentales sobre 8 benchmarks (`fib`, `collatz`, `sort`, `sieve`, `series_pi`, `mandelbrot`, `quicksort` y `opt_heavy`):
* **Tiempo de ejecución (ms):** Mediana de N ejecuciones CPU-bound.
* **Tamaño del binario (bytes):** Peso del ejecutable ELF final.
* **Tiempo de compilación (ms):** Desde la llamada inicial hasta el ejecutable enlazado.

Se compararon las siguientes configuraciones de toolchains:
* **`mio -O0` / `mio -opt`**: Compilador propio sin y con optimizaciones de AST.
* **`g++ -O0` / `g++ -O2`**: GCC en modo desarrollo y producción.
* **`clang++ -O0` / `clang++ -O2`**: Clang/LLVM en modo desarrollo y producción.

### 7.2 Resultados Experimentales Consolidados
La siguiente tabla resume los resultados generales. Los tamaños de binarios corresponden a la media aritmética, y los tiempos de ejecución y compilación a la media geométrica del ratio relativo frente a `g++ -O2` (referencia = 1.00):

| Toolchain | Tamaño medio (bytes) | Ejecución (×) | Compilación (×) |
|---|---|---|---|
| **mio -O0** | 16550 | 6.26 | 0.73 |
| **g++ -O0** | 16001 | 3.19 | 0.88 |
| **clang++ -O0** | 15933 | 3.65 | 1.09 |
| **mio -opt** | 16533 | 5.74 | 0.77 |
| **g++ -O2** | 15981 | 1.00 | 1.00 |
| **clang++ -O2** | 15933 | 0.82 | 1.22 |

*Gráficos de resumen disponibles en el reporte específico: [Ejecución](../benchmarks/results/summary/exec.png), [Compilación](../benchmarks/results/summary/compile.png) y [Tamaño](../benchmarks/results/summary/size.png).*

### 7.3 Discusión Técnica y Análisis de Código
1. **Modelo de Pila vs. Registros (Tiempos de Ejecución):**
   `mio -O0` es **6.26× más lento** que `g++ -O2` y **~2×** más lento que `g++ -O0`. Esto se debe a que `mio` genera código basado en un **modelo de pila (stack-machine)** para evaluar expresiones binarias (múltiples accesos de lectura/escritura a memoria mediante `pushq`, `popq` y offsets negativos respecto a `%rbp`), mientras que GCC/Clang aplican asignación local de registros incluso en `-O0`. Esto genera una alta presión sobre la caché L1d y reduce notablemente el throughput de instrucciones en `mio`.
2. **Impacto de la Optimización AST (`-opt`):**
   En el benchmark de optimización intensiva (`opt_heavy`), `mio -opt` logra una mejora del **40.4%** en tiempo de ejecución (reducción de **371.9 ms a 221.5 ms**) y disminuye en 136 bytes el tamaño del binario. Esto se justifica por la aplicación exitosa de *constant folding*, *constant propagation*, simplificación algebraica (ej. `i * 1 + 0` → `i`) y la eliminación de ramas muertas (`if (1 == 0)`). No obstante, el impacto es nulo en loops de cómputo puro sin constantes propagables, debido a que las optimizaciones operan sobre el AST antes de emitir código y no abordan optimizaciones de backend como la asignación global de registros.
3. **Tiempos de Compilación (Ventaja de Pipeline Directo):**
   `mio` es el compilador más rápido del conjunto, requiriendo en promedio solo el **73%** del tiempo de `g++ -O2`. Esta notable velocidad se debe a un pipeline directo AST → assembly sin representaciones intermedias complejas (IR) ni costosos pases de análisis o asignación global de registros.
4. **Tamaño de Binario:**
   Los ejecutables de `mio` son solo un **3-4%** más grandes que los de GCC y Clang. Esto se debe a la mayor densidad de instrucciones del código de pila y a la inclusión de cadenas de formato estándar redundantes para funciones de salida.

---

## 8. Cómo compilar y ejecutar

**Requisitos:** `g++` con C++17, Python 3, Node.js (solo frontend), Docker
(opcional).

```bash
cd compiler
python3 build.py build                                # compila el compilador
python3 build.py test                                 # corre las pruebas
python3 build.py run --asm tests/analysis/input1.txt  # genera el assembly de un archivo
```

`run` reenvía sus argumentos al compilador, que acepta `--tokens`, `--ast`,
`--json`, `--asm` y `--opt`.

---

## 9. Índice de documentación

Todo el detalle técnico vive en `docs/`:

| Archivo | Contenido |
|---|---|
| `docs/overview.md` | arquitectura general y decisiones de diseño |
| `docs/proyecto.md` | enunciado y rúbrica oficial del proyecto |
| `docs/grammar.md` | gramática (CFG) del subconjunto de C++ |
| `docs/semantic_rules.md` | reglas de tipos, compatibilidad y scope |
| `docs/codegen.md` | plan de generación de código x86-64 |
| `docs/benchmarks.md` | **reporte y evaluación comparativa de benchmarks** |
| `docs/decisiones/` | decisiones de diseño puntuales |
| `docs/guide/` | **guía técnica fase por fase** (lexer → codegen) |

La `docs/guide/` es la referencia más completa: explica el *cómo* está implementada
cada fase, con fragmentos de código comentados.
