# Compilador C++ → x86-64

Compilador de un subconjunto de C++ que genera ensamblador x86-64. Proyecto del curso de Compiladores.

## Compilar y ejecutar

Desde `compiler/`:

```bash
python3 build.py build                                # compila
python3 build.py test                                 # corre todas las pruebas
python3 build.py e2e                                  # solo las pruebas end-to-end
python3 build.py run --asm tests/analysis/input1.txt  # genera el assembly de un archivo
python3 build.py clean                                # limpia el build
```

`run` reenvía sus argumentos al compilador, que acepta `--tokens`, `--ast`, `--json` y `--asm`.

## Estructura

```
compiler/   → compilador (C++)
backend/    → API REST (Python)
frontend/   → app web (React)
benchmarks/ → comparación con GCC/Clang
docs/       → documentación técnica
```

## Documentación

- `docs/overview.md` — arquitectura y decisiones de diseño
- `docs/grammar.md` — gramática CFG del lenguaje
- `docs/proyecto.md` — requerimientos y especificaciones del proyecto

## Requisitos

- g++ con soporte C++17
- Python 3
- Node.js (solo para el frontend)

