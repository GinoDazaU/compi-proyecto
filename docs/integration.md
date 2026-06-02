# Integración Frontend & Backend

Guía rápida para la integración del compilador C++ con el backend y frontend.

## Estructura de Carpetas Sugerida

El proyecto se organiza de la siguiente manera en la raíz:
* `/backend`: Contiene la API REST (Python).
* `/compiler`: Contiene el compilador (C++).
* `/frontend`: Contiene la aplicación web (React/Vite).

---

## Prerrequisito (Compilación inicial)

Antes de iniciar el backend, se debe compilar el compilador **una sola vez** para generar el binario:
1. Ir a `/compiler`.
2. Ejecutar:
   ```bash
   python3 build.py
   ```
Esto creará el ejecutable en `/compiler/build/compiler`.

---

## Flujo de Datos

1. **Frontend (React)**: Envía el código fuente (string) al backend.
2. **Backend (Python)** (ejecutado desde la carpeta `/backend`):
   * Guarda el código en un archivo (ej. `temp.cpp`), sobrescribiendo el anterior.
   * Ejecuta el compilador pasándole el flag `--json`:
     ```bash
     ../compiler/build/compiler --json temp.cpp
     ```
   * Lee la salida de `stdout`.
   * Envía el JSON obtenido al Frontend.
3. **Frontend (React)**: Recibe el JSON y lo renderiza de forma visual.

---

## Backend (Python)
Esquema conceptual para invocar el compilador desde la carpeta `/backend`:

```python
import subprocess
import json

# Ejecutar compilador y capturar stdout usando la ruta relativa
res = subprocess.run(["../compiler/build/compiler", "--json", "temp.cpp"], capture_output=True, text=True)
ast_json = json.loads(res.stdout)
```

---

## Frontend (React)
Librerías recomendadas para renderizar la estructura del AST:
* [react-d3-tree](https://github.com/bkardobe/react-d3-tree) (Árboles jerárquicos interactivos)
* [react-flow](https://reactflow.dev/) (Grafos y nodos interactivos)
