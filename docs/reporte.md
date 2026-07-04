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

## 7. Benchmarks

> _Sección en elaboración — informe a cargo de **_(nombre del compañero)_**._

Comparación experimental del compilador contra herramientas de uso extendido
(**GCC**, **Clang/LLVM** y **Rust**) sobre programas de prueba (fibonacci,
ordenamientos, criba, etc.), midiendo tiempo de compilación, tamaño del binario y
velocidad de ejecución.

Los casos y scripts están en `benchmarks/`. _(El análisis con tablas y gráficos se
adjunta en el documento anexo / se completará en esta sección.)_

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
| `docs/decisiones/` | decisiones de diseño puntuales |
| `docs/guide/` | **guía técnica fase por fase** (lexer → codegen) |

La `docs/guide/` es la referencia más completa: explica el *cómo* está implementada
cada fase, con fragmentos de código comentados.
</content>
</invoke>
