import { useEffect, useState } from "react";
import { Workflow, Braces, Maximize2, Minimize2 } from "lucide-react";
import AstTree from "./AstTree";
import JsonTree from "./JsonTree";

export default function AstView({ ast }) {
  const [view, setView] = useState("tree");
  const [expanded, setExpanded] = useState(false);

  useEffect(() => {
    if (!expanded) return;
    const onKey = (e) => e.key === "Escape" && setExpanded(false);
    document.addEventListener("keydown", onKey);
    return () => document.removeEventListener("keydown", onKey);
  }, [expanded]);

  return (
    <div className={`flex flex-col bg-white dark:bg-stone-900 ${expanded ? "fixed inset-0 z-50" : "h-full"}`}>
      <div className="flex items-center gap-1 border-b border-stone-100 px-3 py-1.5 dark:border-stone-800">
        <Toggle active={view === "tree"} onClick={() => setView("tree")} icon={Workflow}>
          Tree
        </Toggle>
        <Toggle active={view === "json"} onClick={() => setView("json")} icon={Braces}>
          JSON
        </Toggle>

        <button
          onClick={() => setExpanded(!expanded)}
          title={expanded ? "Exit fullscreen (Esc)" : "Fullscreen"}
          className="ml-auto flex items-center rounded-md px-2 py-1 text-stone-500 transition hover:bg-stone-50 dark:text-stone-400 dark:hover:bg-stone-800"
        >
          {expanded ? <Minimize2 className="h-3.5 w-3.5" /> : <Maximize2 className="h-3.5 w-3.5" />}
        </button>
      </div>

      <div className="min-h-0 flex-1">
        {view === "tree" ? (
          <AstTree key={expanded ? "full" : "panel"} ast={ast} />
        ) : (
          <div className="h-full overflow-auto p-3 font-mono text-sm">
            <JsonTree data={ast} defaultOpen />
          </div>
        )}
      </div>
    </div>
  );
}

function Toggle({ active, onClick, icon: Icon, children }) {
  return (
    <button
      onClick={onClick}
      className={`flex items-center gap-1.5 rounded-md px-2.5 py-1 text-xs font-medium transition ${
        active
          ? "bg-green-50 text-green-700 dark:bg-green-900/30 dark:text-green-400"
          : "text-stone-500 hover:bg-stone-50 dark:text-stone-400 dark:hover:bg-stone-800"
      }`}
    >
      <Icon className="h-3.5 w-3.5" />
      {children}
    </button>
  );
}
