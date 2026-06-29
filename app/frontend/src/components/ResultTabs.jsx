import { useState } from "react";
import { Terminal, Cpu, ListTree, Table, FileCode2 } from "lucide-react";
import Console from "./Console";
import AstView from "./AstView";
import TokensTable from "./TokensTable";

const TABS = [
  { id: "Console", icon: Terminal },
  { id: "Assembly", icon: Cpu },
  { id: "AST", icon: ListTree },
  { id: "Tokens", icon: Table },
];

export default function ResultTabs({ result, loading }) {
  const [tab, setTab] = useState("Console");
  const ok = result?.success;

  return (
    <>
      <div className="flex bg-stone-50 dark:bg-stone-800">
        {TABS.map(({ id, icon: Icon }) => (
          <button
            key={id}
            onClick={() => setTab(id)}
            className={`flex items-center gap-1.5 px-4 py-2 text-xs font-medium uppercase tracking-wide transition ${
              tab === id
                ? "border-b-2 border-green-600 bg-white text-stone-800 dark:bg-stone-900 dark:text-stone-100"
                : "border-b-2 border-transparent text-stone-400 hover:text-stone-600 dark:text-stone-500 dark:hover:text-stone-300"
            }`}
          >
            <Icon className="h-3 w-3" />
            {id}
          </button>
        ))}
      </div>

      <div className="min-h-0 flex-1 overflow-hidden bg-white dark:bg-stone-900">
        {tab === "AST" && ok ? (
          <AstView ast={result.ast} />
        ) : (
          <div className="h-full overflow-auto p-3 font-mono text-sm">
            {tab === "Console" ? (
              <Console result={result} loading={loading} />
            ) : !ok ? (
              <Empty />
            ) : tab === "Assembly" ? (
              <pre className="leading-relaxed text-stone-800 dark:text-stone-200">{result.asm}</pre>
            ) : (
              <TokensTable tokens={result.tokens} />
            )}
          </div>
        )}
      </div>
    </>
  );
}

function Empty() {
  return (
    <div className="flex h-full flex-col items-center justify-center gap-3 text-center text-sm text-stone-400 dark:text-stone-500">
      <FileCode2 className="h-8 w-8 text-stone-300 dark:text-stone-600" />
      Run a program to see the assembly, AST and tokens.
    </div>
  );
}
