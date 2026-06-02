import { useRef, useEffect } from 'react';
import { EditorState } from '@codemirror/state';
import { EditorView, keymap, lineNumbers, highlightActiveLine, highlightActiveLineGutter } from '@codemirror/view';
import { defaultKeymap, indentWithTab } from '@codemirror/commands';
import { cpp } from '@codemirror/lang-cpp';
import { oneDark } from '@codemirror/theme-one-dark';
import { searchKeymap, highlightSelectionMatches } from '@codemirror/search';
import { bracketMatching, indentOnInput } from '@codemirror/language';

const SAMPLE_CODE = `
int factorial(int n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

int main() {
    int x = factorial(5);
    println(x);
    return 0;
}
`;

export default function CodeEditor({ initialCode, onCodeChange }) {
  const containerRef = useRef(null);
  const viewRef = useRef(null);

  useEffect(() => {
    if (!containerRef.current) return;

    const state = EditorState.create({
      doc: initialCode || SAMPLE_CODE,
      extensions: [
        lineNumbers(),
        highlightActiveLine(),
        highlightActiveLineGutter(),
        bracketMatching(),
        indentOnInput(),
        highlightSelectionMatches(),
        keymap.of([...defaultKeymap, ...searchKeymap, indentWithTab]),
        cpp(),
        oneDark,
        EditorView.updateListener.of((update) => {
          if (update.docChanged && onCodeChange) {
            onCodeChange(update.state.doc.toString());
          }
        }),
        EditorView.theme({
          '&': { height: '100%' },
          '.cm-scroller': { fontFamily: 'var(--font-mono)', fontSize: '13px' },
        }),
      ],
    });

    const view = new EditorView({ state, parent: containerRef.current });
    viewRef.current = view;

    // Report initial code
    if (onCodeChange) onCodeChange(state.doc.toString());

    return () => view.destroy();
  }, []); // eslint-disable-line react-hooks/exhaustive-deps

  return <div ref={containerRef} className="editor-container" />;
}
