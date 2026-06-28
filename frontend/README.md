# Frontend — Compilador C++ → x86-64

App web (Vite + React + Tailwind v4 + CodeMirror) para escribir código, compilarlo
y ver tokens, AST, assembly y el output de ejecución.

## Desarrollo

Necesita el backend corriendo en `:8000` (Vite hace proxy de `/api`).

```bash
npm install
npm run dev
```

## Estructura

```
src/
  App.jsx            layout y estado
  api.js             llamadas a /api
  index.css          estilos base (el tamaño de letra global se ajusta aquí)
  components/
    Toolbar.jsx      título, toggle --opt, botón Run
    Editor.jsx       editor CodeMirror (C++)
    ResultTabs.jsx   pestañas Consola / Assembly / AST / Tokens
    Console.jsx      output o error de la ejecución
    JsonTree.jsx     árbol colapsable del AST
    TokensTable.jsx  tabla de tokens
    MetricsBar.jsx   barra inferior con tiempos y tamaño del binario
```
