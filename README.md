# Compilador C++ → x86-64

Compilador de un subconjunto de C++ que genera ensamblador x86-64. Proyecto del curso de Compiladores.

## Compilador

Desde `compiler/`:

```bash
python3 build.py build                                # compila
python3 build.py test                                 # corre todas las pruebas
python3 build.py run --asm tests/analysis/input1.txt  # genera el assembly de un archivo
```

`run` reenvía sus argumentos al compilador, que acepta `--tokens`, `--ast`, `--json`, `--asm` y `--opt`.

## App web

Cada carpeta (`backend/`, `frontend/`) puede levantarse por separado, o juntos con Docker:

```bash
docker build -t compi . && docker run -p 8000:8000 compi
```

Disponible en `localhost:8000`.

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
- `docs/semantic_rules.md` — reglas del analizador semántico
- `docs/codegen.md` — notas de generación de código
- `docs/proyecto.md` — requerimientos y especificaciones del proyecto
- `docs/guide/` — guía técnica por fase (lexer, parser, semántico, optimizador, codegen)

## Requisitos

- g++ con soporte C++17
- Python 3
- Node.js (solo para el frontend)
- Docker (opcional)
