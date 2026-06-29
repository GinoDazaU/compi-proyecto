import { useEffect, useRef, useState } from "react";
import { BookOpen, ChevronDown } from "lucide-react";
import { EXAMPLES } from "../examples";

export default function ExampleMenu({ onSelect }) {
  const [open, setOpen] = useState(false);
  const ref = useRef(null);

  // Cierra el menú al hacer click fuera de él.
  useEffect(() => {
    if (!open) return;
    const onClickOutside = (e) => {
      if (ref.current && !ref.current.contains(e.target)) setOpen(false);
    };
    document.addEventListener("mousedown", onClickOutside);
    return () => document.removeEventListener("mousedown", onClickOutside);
  }, [open]);

  return (
    <div ref={ref} className="relative">
      <button
        onClick={() => setOpen(!open)}
        className="flex items-center gap-1 rounded px-2 py-0.5 text-xs font-medium normal-case tracking-normal text-stone-500 transition hover:bg-stone-100 hover:text-stone-700"
      >
        <BookOpen className="h-3.5 w-3.5" />
        Examples
        <ChevronDown className={`h-3.5 w-3.5 transition ${open ? "rotate-180" : ""}`} />
      </button>

      {open && (
        <div className="absolute right-0 z-10 mt-1 max-h-72 w-56 overflow-y-auto rounded-md border border-stone-200 bg-white py-1 shadow-lg">
          {EXAMPLES.map((ex) => (
            <button
              key={ex.name}
              onClick={() => {
                onSelect(ex.code);
                setOpen(false);
              }}
              className="block w-full px-3 py-1.5 text-left text-sm normal-case tracking-normal text-stone-700 transition hover:bg-stone-50"
            >
              {ex.name}
            </button>
          ))}
        </div>
      )}
    </div>
  );
}
