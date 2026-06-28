import { useCallback, useState } from "react";
import { FileCode2 } from "lucide-react";
import Toolbar from "./components/Toolbar";
import Editor from "./components/Editor";
import ResultTabs from "./components/ResultTabs";
import MetricsBar from "./components/MetricsBar";
import { runCode } from "./api";

const DEFAULT_CODE = `int main() {
    int a = 0;
    int b = 1;
    for (int i = 0; i < 10; i = i + 1) {
        print(a);
        print(' ');
        int next = a + b;
        a = b;
        b = next;
    }
    println(' ');
    return 0;
}
`;

export default function App() {
  const [code, setCode] = useState(DEFAULT_CODE);
  const [optimize, setOptimize] = useState(false);
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);

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
    <div className="flex h-full flex-col bg-stone-100 text-stone-900">
      <Toolbar onRun={run} loading={loading} optimize={optimize} setOptimize={setOptimize} />

      <div className="flex min-h-0 flex-1">
        <section className="flex min-w-0 flex-1 flex-col border-r border-stone-200">
          <PanelHeader>
            <FileCode2 className="h-3 w-3" />
            Editor
          </PanelHeader>
          <div className="min-h-0 flex-1 overflow-auto bg-white">
            <Editor value={code} onChange={setCode} onRun={run} />
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

function PanelHeader({ children }) {
  return (
    <div className="flex items-center gap-1.5 border-b border-stone-200 bg-stone-50 px-3 py-2 text-xs font-medium uppercase tracking-wide text-stone-500">
      {children}
    </div>
  );
}
