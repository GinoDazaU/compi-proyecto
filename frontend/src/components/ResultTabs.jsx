import { useState } from "react";
import { Terminal, Cpu, ListTree, Table, FileCode2 } from "lucide-react";
import Console from "./Console";
import JsonTree from "./JsonTree";
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
      <div className="flex border-b border-stone-200 bg-stone-50">
        {TABS.map(({ id, icon: Icon }) => (
          <button
            key={id}
            onClick={() => setTab(id)}
            className={`flex items-center gap-1.5 px-4 py-2 text-xs font-medium uppercase tracking-wide transition ${
              tab === id
                ? "border-b-2 border-green-600 bg-white text-stone-800"
                : "border-b-2 border-transparent text-stone-400 hover:text-stone-600"
            }`}
          >
            <Icon className="h-3 w-3" />
            {id}
          </button>
        ))}
      </div>

      <div className="min-h-0 flex-1 overflow-auto bg-white p-3 font-mono text-sm">
        {tab === "Console" ? (
          <Console result={result} loading={loading} />
        ) : !ok ? (
          <Empty />
        ) : tab === "Assembly" ? (
          <pre className="leading-relaxed text-stone-800">{result.asm}</pre>
        ) : tab === "AST" ? (
          <JsonTree data={result.ast} defaultOpen />
        ) : (
          <TokensTable tokens={result.tokens} />
        )}
      </div>
    </>
  );
}

function Empty() {
  return (
    <div className="flex h-full flex-col items-center justify-center gap-3 text-center text-sm text-stone-400">
      <FileCode2 className="h-8 w-8 text-stone-300" />
      Run a program to see the assembly, AST and tokens.
    </div>
  );
}
