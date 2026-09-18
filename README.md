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

## App Web (Serverless)

La aplicación web corre 100% en el navegador usando WebAssembly para compilar el código C++ a ensamblador x86-64 y simulando la ejecución en un entorno nativo simulado en JavaScript.

Desde `app/frontend/`:

```bash
npm install
npm run dev
```

La app usa **Vite** y puede ser desplegada automáticamente en **Vercel** subiendo la carpeta `app/frontend` como directorio raíz.

## Estructura

```
compiler/   → compilador (C++)
app/        → frontend (React + WebAssembly + Simulador x86)
benchmarks/ → comparación con GCC, Clang y Rust
docs/       → documentación del proyecto
```

## Documentación

- `docs/overview.md` — arquitectura y decisiones de diseño
- `docs/grammar.md` — gramática CFG del lenguaje
- `docs/semantic_rules.md` — reglas del analizador semántico
- `docs/codegen.md` — notas de generación de código
- `docs/proyecto.md` — requerimientos y especificaciones del proyecto
- `docs/guide/` — guía técnica por fase (lexer, parser, semántico, optimizador, codegen)

## Requisitos

- g++ con soporte C++17 (para desarrollo local)
- Emscripten (para compilar WebAssembly en `app/frontend/wasm`)
- Node.js (para correr/desplegar el frontend)
