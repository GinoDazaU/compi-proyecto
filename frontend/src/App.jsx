import { useState, useRef, useCallback } from 'react';
import CodeEditor from './components/CodeEditor';
import TokensPanel from './components/TokensPanel';
import ASTViewer from './components/ASTViewer';
import OutputConsole from './components/OutputConsole';
import { compileCode } from './api/compiler';

export default function App() {
  const [tokens, setTokens] = useState(null);
  const [ast, setAst] = useState(null);
  const [messages, setMessages] = useState([]);
  const [status, setStatus] = useState('idle'); // idle | loading | success | error
  const codeRef = useRef('');

  const handleCodeChange = useCallback((code) => {
    codeRef.current = code;
  }, []);

  const handleCompile = async () => {
    const code = codeRef.current;
    if (!code.trim()) {
      setMessages([{ type: 'warning', text: 'El editor está vacío.' }]);
      return;
    }

    setStatus('loading');
    setMessages([{ type: 'info', text: 'Compilando...' }]);
    setTokens(null);
    setAst(null);

    try {
      const data = await compileCode(code);

      if (data.success) {
        setTokens(data.tokens || []);
        setAst(data.ast || null);
        setStatus('success');

        const tokenCount = (data.tokens || []).filter(t => t.type !== 'END').length;
        setMessages([
          { type: 'success', text: `Compilación exitosa.` },
          { type: 'info', text: `${tokenCount} tokens generados.` },
          { type: 'info', text: `AST generado correctamente.` },
        ]);
      } else {
        setStatus('error');
        const err = data.error || {};
        const location = err.line ? ` [línea ${err.line}:${err.col}]` : '';
        setMessages([
          { type: 'error', text: `Error ${err.type || 'desconocido'}${location}` },
          { type: 'error', text: err.message || 'Error de compilación' },
        ]);
      }
    } catch (err) {
      setStatus('error');
      setMessages([
        { type: 'error', text: 'Error de conexión con el servidor.' },
        { type: 'info', text: 'Asegúrate de que el backend esté corriendo en el puerto 8000.' },
        { type: 'info', text: `Detalle: ${err.message}` },
      ]);
    }
  };

  const handleKeyDown = (e) => {
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
      e.preventDefault();
      handleCompile();
    }
  };

  return (
    <div className="app" onKeyDown={handleKeyDown}>
      {/* Header */}
      <header className="header">
        <div className="header__logo">
          <div className="header__icon">C++</div>
          <div>
            <span className="header__title">Compiler</span>
            <span className="header__subtitle">→ x86-64</span>
          </div>
        </div>

        <div className="header__actions">
          <div className="status">
            <div className={`status__dot status__dot--${status === 'loading' ? 'loading' : status}`} />
            <span>
              {status === 'idle' && 'Listo'}
              {status === 'loading' && 'Compilando...'}
              {status === 'success' && 'Compilado'}
              {status === 'error' && 'Error'}
            </span>
          </div>

          <button
            id="compile-btn"
            className={`btn-compile ${status === 'loading' ? 'btn-compile--loading' : ''}`}
            onClick={handleCompile}
            disabled={status === 'loading'}
          >
            {status === 'loading' ? '' : '▶'}
            {status === 'loading' ? 'Compilando...' : 'Compilar'}
          </button>
        </div>
      </header>

      {/* Main 4-panel grid */}
      <main className="main-grid">
        {/* Top-left: Code Editor */}
        <section className="panel" id="editor-panel">
          <div className="panel__header">
            <div className="panel__title">
              <span className="panel__title-icon">📝</span>
              Editor de Código
            </div>
            <span className="panel__badge">Ctrl+Enter para compilar</span>
          </div>
          <div className="panel__body">
            <CodeEditor onCodeChange={handleCodeChange} />
          </div>
        </section>

        {/* Top-right: AST Viewer */}
        <section className="panel" id="ast-panel">
          <div className="panel__header">
            <div className="panel__title">
              <span className="panel__title-icon">🌳</span>
              AST (Abstract Syntax Tree)
            </div>
            {ast && <span className="panel__badge">Zoom: scroll · Mover: click+arrastrar</span>}
          </div>
          <div className="panel__body">
            <ASTViewer ast={ast} />
          </div>
        </section>

        {/* Bottom-left: Tokens */}
        <section className="panel" id="tokens-panel">
          <div className="panel__header">
            <div className="panel__title">
              <span className="panel__title-icon">🔤</span>
              Tokens
            </div>
            {tokens && (
              <span className="panel__badge">
                {tokens.filter(t => t.type !== 'END').length} tokens
              </span>
            )}
          </div>
          <div className="panel__body">
            <TokensPanel tokens={tokens} />
          </div>
        </section>

        {/* Bottom-right: Output Console */}
        <section className="panel" id="output-panel">
          <div className="panel__header">
            <div className="panel__title">
              <span className="panel__title-icon">💻</span>
              Consola
            </div>
          </div>
          <div className="panel__body">
            <OutputConsole messages={messages} status={status} />
          </div>
        </section>
      </main>
    </div>
  );
}
