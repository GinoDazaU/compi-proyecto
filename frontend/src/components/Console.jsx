// Contenido de la pestaña "Consola": output del programa o el error.
export default function Console({ result, loading }) {
  if (loading) return <Muted>Running…</Muted>;
  if (!result) return <Muted>Press Run to compile and execute.</Muted>;

  if (!result.success) {
    const e = result.error;
    return (
      <div className="text-red-600">
        <span className="font-semibold">error {e.type}</span>
        {e.line > 0 && <span className="text-red-400"> at {e.line}:{e.col}</span>}
        {": "}
        {e.message}
      </div>
    );
  }

  const run = result.run;
  if (!run) return <Muted>Compiled without executing.</Muted>;

  return (
    <div className="whitespace-pre-wrap">
      {run.stdout && <span className="text-stone-800">{run.stdout}</span>}
      {run.stderr && <span className="text-red-600">{run.stderr}</span>}
      {!run.stdout && !run.stderr && <Muted>(no output)</Muted>}
    </div>
  );
}

function Muted({ children }) {
  return <span className="text-stone-400">{children}</span>;
}
