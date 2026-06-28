import { useState } from "react";

// Árbol colapsable para cualquier JSON (usado para el AST).
export default function JsonTree({ data, name, defaultOpen = false }) {
  const [open, setOpen] = useState(defaultOpen);

  if (data === null || typeof data !== "object") {
    return (
      <div className="leading-6">
        {name && <Key>{name}: </Key>}
        <Value value={data} />
      </div>
    );
  }

  const isArray = Array.isArray(data);
  const entries = Object.entries(data);
  // Para nodos del AST, mostrar el "type" junto a la llave da contexto.
  const tag = !isArray && typeof data.type === "string" ? data.type : null;

  return (
    <div className="leading-6">
      <button
        onClick={() => setOpen(!open)}
        className="cursor-pointer text-left text-stone-600 hover:text-stone-900"
      >
        <span className="inline-block w-4 text-stone-400">{open ? "▾" : "▸"}</span>
        {name && <Key>{name}: </Key>}
        {tag ? (
          <span className="font-semibold text-green-700">{tag}</span>
        ) : (
          <span className="text-stone-400">{isArray ? `[${entries.length}]` : "{…}"}</span>
        )}
      </button>

      {open && (
        <div className="ml-4 border-l border-stone-200 pl-3">
          {entries.map(([k, v]) => (
            <JsonTree key={k} name={isArray ? `[${k}]` : k} data={v} />
          ))}
        </div>
      )}
    </div>
  );
}

function Key({ children }) {
  return <span className="text-stone-500">{children}</span>;
}

function Value({ value }) {
  const cls =
    typeof value === "string"
      ? "text-green-700"
      : typeof value === "number"
        ? "text-blue-700"
        : "text-stone-500";
  return <span className={cls}>{JSON.stringify(value)}</span>;
}
