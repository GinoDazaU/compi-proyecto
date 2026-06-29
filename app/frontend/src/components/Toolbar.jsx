import { Play, Loader2, Zap, Moon, Sun } from "lucide-react";

export default function Toolbar({ onRun, loading, optimize, setOptimize, dark, toggleDark }) {
  return (
    <header className="grid grid-cols-3 items-center border-b border-stone-200 bg-white px-4 py-2 shadow-sm dark:border-stone-700 dark:bg-stone-900">
      {/* Izquierda: título */}
      <h1 className="text-base font-semibold text-stone-800 dark:text-stone-100">
        C++ Compiler <span className="text-stone-400">→</span> x86-64
      </h1>

      {/* Centro: split button Run | Optimize */}
      <div className="flex justify-center">
        <div className="flex overflow-hidden rounded-lg shadow-sm">
          <button
            onClick={onRun}
            disabled={loading}
            title="Run (Ctrl+Enter)"
            className="flex items-center gap-2 bg-green-600 px-5 py-1.5 text-sm font-semibold text-white transition hover:bg-green-700 active:scale-[0.98] disabled:cursor-not-allowed disabled:opacity-60"
          >
            {loading ? (
              <Loader2 className="h-4 w-4 animate-spin" />
            ) : (
              <Play className="h-4 w-4 fill-current" />
            )}
            {loading ? "Running…" : "Run"}
          </button>
          <div className="w-px bg-green-500" />
          <button
            onClick={() => setOptimize(!optimize)}
            title="Toggle optimizations"
            className={`flex items-center px-3 py-1.5 transition ${
              optimize
                ? "bg-green-600 text-white hover:bg-green-700"
                : "bg-green-600 text-green-200 hover:bg-green-700 hover:text-white"
            }`}
          >
            <Zap className={`h-4 w-4 ${optimize ? "fill-current" : ""}`} />
          </button>
        </div>
      </div>

      {/* Derecha: modo oscuro */}
      <div className="flex justify-end">
        <button
          onClick={toggleDark}
          title={dark ? "Light mode" : "Dark mode"}
          className="rounded-md border border-stone-200 p-1.5 text-stone-500 transition hover:bg-stone-100 hover:text-stone-700 dark:border-stone-600 dark:text-stone-400 dark:hover:bg-stone-800 dark:hover:text-stone-200"
        >
          {dark ? <Sun className="h-4 w-4" /> : <Moon className="h-4 w-4" />}
        </button>
      </div>
    </header>
  );
}
