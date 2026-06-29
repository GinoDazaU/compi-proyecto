# Backend — API REST del compilador

Expone el compilador C++ → x86-64 a la app web: recibe código fuente, lo compila,
lo ensambla con `g++`, lo ejecuta y devuelve tokens, AST, assembly, métricas y el
output de ejecución.

## Levantar

El backend compila el compilador solo al arrancar (vía `compiler/build.py`), así
que basta con preparar el entorno de Python:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python main.py
```

Queda en `http://localhost:8000`. El frontend (Vite) hace proxy de `/api` hacia aquí.

## Endpoints

### `GET /api/health`

Estado del servicio.

```json
{ "status": "ok", "compiler_ready": true }
```

### `POST /api/run`

Compila, ensambla y ejecuta.

Request:

```json
{ "code": "int main() { return 0; }", "optimize": false }
```

Response:

```jsonc
{
  "success": true,
  "tokens": [ { "type": "...", "lexeme": "...", "line": 1, "col": 1 } ],
  "ast": { "...": "..." },
  "asm": ".data\n...",              // assembly x86-64 generado
  "error": null,                    // si falla: { type, line, col, message }
  "run": {                          // presente si llegó a ejecutar
    "stdout": "...", "stderr": "", "exit_code": 0, "timed_out": false
  },
  "metrics": {                      // tiempos por fase y tamaño del binario
    "compile_ms": 0, "assemble_ms": 0, "exec_ms": 0, "binary_size_bytes": 0
  }
}
```

Si la compilación falla, `success` es `false` y `error.type` es uno de
`lexical | syntax | semantic | server`.

## Notas

- Si la compilación del compilador al arrancar falla, el backend igual levanta y
  `/api/health` reporta `compiler_ready: false`.
- Timeouts: 10s para compilar, 15s para ejecutar el programa. El output se recorta
  a 64 KB.
