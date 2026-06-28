export default function TokensTable({ tokens }) {
  if (!tokens?.length) {
    return <div className="p-3 text-sm text-stone-400">No tokens.</div>;
  }

  return (
    <table className="w-full border-collapse text-left">
      <thead className="sticky top-0 bg-stone-50 text-xs uppercase tracking-wide text-stone-500">
        <tr>
          <th className="px-3 py-1.5 font-medium">Type</th>
          <th className="px-3 py-1.5 font-medium">Lexeme</th>
          <th className="px-3 py-1.5 font-medium">Line</th>
          <th className="px-3 py-1.5 font-medium">Col</th>
        </tr>
      </thead>
      <tbody>
        {tokens.map((t, i) => (
          <tr key={i} className="border-t border-stone-100">
            <td className="px-3 py-1 text-green-700">{t.type}</td>
            <td className="px-3 py-1 text-stone-800">{t.lexeme}</td>
            <td className="px-3 py-1 text-stone-500">{t.line}</td>
            <td className="px-3 py-1 text-stone-500">{t.col}</td>
          </tr>
        ))}
      </tbody>
    </table>
  );
}
