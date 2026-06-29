import { useCallback, useState } from "react";
import { FileCode2 } from "lucide-react";
import Toolbar from "./components/Toolbar";
import Editor from "./components/Editor";
import ExampleMenu from "./components/ExampleMenu";
import ResultTabs from "./components/ResultTabs";
import MetricsBar from "./components/MetricsBar";
import { runCode } from "./api";

const DEFAULT_CODE = `int main() {
    println("Hello, World!");
    int sum = 0;
    for (int i = 1; i <= 10; i = i + 1) {
        sum = sum + i;
    }
    print("Sum 1..10 = ");
    println(sum);
    return 0;
}
`;

export default function App() {
  const [code, setCode] = useState(DEFAULT_CODE);
  const [optimize, setOptimize] = useState(false);
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);
  const [dark, setDark] = useState(() => {
    const saved = localStorage.getItem("dark");
    const isDark = saved === null ? true : saved === "true";
    document.documentElement.classList.toggle("dark", isDark);
    return isDark;
  });

  const toggleDark = useCallback(() => {
    setDark((d) => {
      document.documentElement.classList.toggle("dark", !d);
      localStorage.setItem("dark", String(!d));
      return !d;
    });
  }, []);

  const run = useCallback(async () => {
    setLoading(true);
    try {
      setResult(await runCode(code, optimize));
    } catch {
      setResult({
        success: false,
        error: { type: "server", line: 0, col: 0, message: "Could not reach the backend." },
      });
    } finally {
      setLoading(false);
    }
  }, [code, optimize]);

  return (
    <div className="flex h-full flex-col bg-stone-100 text-stone-900 dark:bg-stone-900 dark:text-stone-100">
      <Toolbar onRun={run} loading={loading} optimize={optimize} setOptimize={setOptimize} dark={dark} toggleDark={toggleDark} />

      <div className="flex min-h-0 flex-1">
        <section className="flex min-w-0 flex-1 flex-col border-r border-stone-200 dark:border-stone-700">
          <div className="flex items-center justify-between border-b-2 border-stone-200 bg-stone-50 px-3 py-1.5 dark:border-stone-700 dark:bg-stone-800">
            <span className="flex items-center gap-1.5 text-xs font-medium uppercase tracking-wide text-stone-500 dark:text-stone-400">
              <FileCode2 className="h-3 w-3" />
              Editor
            </span>
            <ExampleMenu onSelect={setCode} />
          </div>
          <div className="min-h-0 flex-1 overflow-auto bg-white dark:bg-stone-900">
            <Editor value={code} onChange={setCode} onRun={run} dark={dark} />
          </div>
        </section>

        <section className="flex w-1/2 min-w-0 flex-col">
          <ResultTabs result={result} loading={loading} />
        </section>
      </div>

      <MetricsBar result={result} />
    </div>
  );
}
