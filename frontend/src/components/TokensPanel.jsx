/**
 * Classifies a token type string into a visual category for coloring.
 */
function tokenCategory(type) {
  if (type.startsWith('KW_')) return 'keyword';
  if (type.endsWith('_LIT') || type === 'KW_TRUE' || type === 'KW_FALSE') return 'literal';
  if (type === 'STRING_LIT') return 'string';
  if (type === 'ID') return 'identifier';
  if (['LPAREN','RPAREN','LBRACE','RBRACE','LBRACKET','RBRACKET','SEMICOLON','COMMA','COLON'].includes(type)) return 'delimiter';
  if (['END','ERR'].includes(type)) return 'special';
  return 'operator';
}

export default function TokensPanel({ tokens }) {
  if (!tokens || tokens.length === 0) {
    return (
      <div className="empty-state">
        <div className="empty-state__icon">🔤</div>
        <div className="empty-state__text">
          Compilá tu código para ver los tokens
        </div>
      </div>
    );
  }

  return (
    <table className="tokens-table">
      <thead>
        <tr>
          <th>Tipo</th>
          <th>Lexema</th>
          <th>Pos</th>
        </tr>
      </thead>
      <tbody>
        {tokens.map((tok, i) => {
          const cat = tokenCategory(tok.type);
          return (
            <tr key={i}>
              <td className={`token-type token-type--${cat}`}>{tok.type}</td>
              <td><span className="token-lexeme">{tok.lexeme || '⏎'}</span></td>
              <td className="token-pos">{tok.line}:{tok.col}</td>
            </tr>
          );
        })}
      </tbody>
    </table>
  );
}
