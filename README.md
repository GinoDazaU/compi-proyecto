# Compilador C++ → x86-64

Compilador de un subconjunto de C++ que genera ensamblador x86-64. Proyecto del curso de Compiladores.

## Requisitos

- g++ con soporte C++17
- Python 3
- Node.js (solo para el frontend)

## Estructura

```
compiler/   → compilador (C++)
backend/    → API REST (Python)
frontend/   → app web (React)
benchmarks/ → comparación con GCC/Clang
docs/       → documentación técnica
```

## Compilar y ejecutar

Desde `compiler/`:

```bash
python build.py                                  # compila
python build.py run tests/input/<archivo>.txt    # compila y ejecuta
python build.py clean                            # limpia el build
```

## Documentación

- `docs/overview.md` — arquitectura y decisiones de diseño
- `docs/grammar.md` — gramática CFG del lenguaje
