export default function OutputConsole({ messages, status }) {
  if (!messages || messages.length === 0) {
    return (
      <div className="console__empty">
        <div className="console__empty-icon">⌨️</div>
        <div>Presiona <strong>Compilar</strong> para ver los resultados</div>
      </div>
    );
  }

  return (
    <div className="console">
      {messages.map((msg, i) => (
        <div key={i} className={`console__line console__line--${msg.type}`}>
          <span className="console__prefix">
            {msg.type === 'error' ? '✗' : msg.type === 'success' ? '✓' : msg.type === 'warning' ? '⚠' : '›'}
          </span>
          <span>{msg.text}</span>
        </div>
      ))}
    </div>
  );
}
