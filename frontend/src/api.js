// Cliente del backend. En desarrollo, vite hace proxy de /api al FastAPI.

async function post(path, body) {
  const res = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}

// Compila y ejecuta: devuelve tokens, AST, assembly, métricas y el output.
export function runCode(code, optimize) {
  return post("/api/run", { code, optimize });
}
