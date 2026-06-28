import { Play, Loader2, Zap } from "lucide-react";

export default function Toolbar({ onRun, loading, optimize, setOptimize }) {
  return (
    <header className="grid grid-cols-3 items-center border-b border-stone-200 bg-white px-4 py-2 shadow-sm">
      {/* Izquierda: título */}
      <h1 className="text-sm font-semibold text-stone-800">
        C++ Compiler <span className="text-stone-400">→</span> x86-64
      </h1>

      {/* Centro: Run */}
      <div className="flex justify-center">
        <button
          onClick={onRun}
          disabled={loading}
          title="Run (Ctrl+Enter)"
          className="flex items-center gap-2 rounded-lg border border-transparent bg-green-600 px-5 py-1.5 text-sm font-semibold text-white shadow-sm transition hover:bg-green-700 active:scale-[0.98] disabled:cursor-not-allowed disabled:opacity-60"
        >
          {loading ? (
            <Loader2 className="h-4 w-4 animate-spin" />
          ) : (
            <Play className="h-4 w-4 fill-current" />
          )}
          {loading ? "Running…" : "Run"}
        </button>
      </div>

      {/* Derecha: optimización */}
      <div className="flex justify-end">
        <button
          onClick={() => setOptimize(!optimize)}
          title="Enable optimizations (--opt)"
          className={`flex items-center gap-1.5 rounded-lg border px-3 py-1.5 text-sm font-medium transition ${
            optimize
              ? "border-green-300 bg-green-50 text-green-700"
              : "border-stone-200 bg-white text-stone-500 hover:bg-stone-50"
          }`}
        >
          <Zap className={`h-4 w-4 ${optimize ? "fill-current" : ""}`} />
          Optimize
        </button>
      </div>
    </header>
  );
}
