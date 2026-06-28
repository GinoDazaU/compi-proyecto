import CodeMirror from "@uiw/react-codemirror";
import { cpp } from "@codemirror/lang-cpp";
import { keymap } from "@codemirror/view";
import { Prec } from "@codemirror/state";

export default function Editor({ value, onChange, onRun }) {
  // Ctrl/Cmd+Enter ejecuta sin tener que ir al botón.
  const runKeymap = Prec.highest(
    keymap.of([{ key: "Mod-Enter", run: () => (onRun(), true) }]),
  );

  return (
    <CodeMirror
      value={value}
      onChange={onChange}
      extensions={[cpp(), runKeymap]}
      height="100%"
      className="h-full"
      basicSetup={{ tabSize: 4 }}
    />
  );
}
