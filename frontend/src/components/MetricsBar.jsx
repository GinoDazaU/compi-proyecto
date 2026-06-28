import { Gauge, Check, X } from "lucide-react";

// Barra de estado inferior: tiempos por fase, tamaño del binario y exit code.
export default function MetricsBar({ result }) {
  const m = result?.metrics;
  const run = result?.run;

  const items = [];
  if (m?.compile_ms != null) items.push(["compile", `${m.compile_ms} ms`]);
  if (m?.assemble_ms != null) items.push(["assemble", `${m.assemble_ms} ms`]);
  if (m?.exec_ms != null) items.push(["exec", `${m.exec_ms} ms`]);
  if (m?.binary_size_bytes != null) items.push(["binary", fmtBytes(m.binary_size_bytes)]);

  const exitOk = run && run.exit_code === 0 && !run.timed_out;

  return (
    <div className="flex h-8 items-center gap-4 border-t border-stone-300 bg-stone-50 px-4 text-xs text-stone-500">
      <Gauge className="h-3.5 w-3.5 text-stone-400" />

      {items.length === 0 ? (
        <span className="text-stone-400">No metrics yet</span>
      ) : (
        items.map(([k, v]) => (
          <span key={k}>
            {k} <span className="font-medium text-stone-700">{v}</span>
          </span>
        ))
      )}

      {run && (
        <span
          className={`ml-auto flex items-center gap-1 ${exitOk ? "text-stone-500" : "text-red-500"}`}
        >
          {exitOk ? <Check className="h-3.5 w-3.5" /> : <X className="h-3.5 w-3.5" />}
          exit {run.exit_code}
          {run.timed_out && " (timeout)"}
        </span>
      )}
    </div>
  );
}

function fmtBytes(b) {
  return b < 1024 ? `${b} B` : `${(b / 1024).toFixed(1)} KB`;
}
