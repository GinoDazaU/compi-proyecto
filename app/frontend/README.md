# Frontend — Compilador C++ → x86-64

App web (Vite + React + Tailwind v4 + CodeMirror) para escribir código, compilarlo y ver tokens, AST, assembly y el output de ejecución. Totalmente **Serverless**.

## Arquitectura

- **Compilador en WASM:** El código fuente en C++ del compilador (`../../compiler`) se ha transpuesto a WebAssembly (usando Emscripten). La compilación de C++ a Assembly se ejecuta **localmente en tu navegador**, sin llamadas a un servidor externo.
- **Simulador x86-64:** El assembly AT&T emitido por el compilador es ejecutado instrucción por instrucción por un simulador virtual escrito en JavaScript puro, con soporte de registros, stack, heap (`malloc`), banderas (`EFLAGS`) y llamadas matemáticas de punto flotante.

## Desarrollo

```bash
npm install
npm run dev
```

*(Si editas el compilador en C++, recuerda recompilar el módulo `.wasm` usando el script en la carpeta `wasm/`)*.

## Estructura

```
wasm/
  compiler_wrapper.cpp   envoltorio C para exponer la API C++ a JS
  build_wasm.sh          script Emscripten para construir el compiler.wasm
public/
  wasm/
    compiler.js          loader generado por Emscripten
    compiler.wasm        el binario del compilador
src/
  App.jsx                layout y estado
  api.js                 llama al módulo WASM e inicia el simulador
  simulator.js           simulador x86-64 y memoria virtual en JavaScript puro
  index.css              estilos base
  components/
    Toolbar.jsx          título, toggle --opt, botón Run
    Editor.jsx           editor CodeMirror (C++)
    ResultTabs.jsx       pestañas Consola / Assembly / AST / Tokens
    Console.jsx          output o error de la ejecución
    JsonTree.jsx         árbol colapsable del AST
    TokensTable.jsx      tabla de tokens
    MetricsBar.jsx       barra inferior con tiempos y métricas
```
