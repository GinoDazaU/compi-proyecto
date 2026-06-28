// Convierte el AST (JSON del compilador) al formato de react-d3-tree:
// { name, attributes, children }. El nombre es el "type" del nodo; los campos
// primitivos van como attributes y los nodos anidados como children, en orden.

export function astToTree(node) {
  return build(node);
}

function build(value) {
  const out = { name: typeName(value), attributes: {}, children: [] };

  if (value && typeof value === "object" && !Array.isArray(value)) {
    for (const [key, v] of Object.entries(value)) {
      if (key === "type") continue;
      addField(out, key, v);
    }
  }
  return out;
}

function addField(out, key, v) {
  if (v === null || v === undefined) return;

  if (Array.isArray(v)) {
    for (const item of v) {
      if (item && typeof item === "object") out.children.push(build(item));
    }
  } else if (typeof v === "object") {
    out.children.push(build(v));
  } else {
    out.attributes[key] = v;
  }
}

function typeName(value) {
  if (value && typeof value === "object" && typeof value.type === "string") {
    return value.type;
  }
  return "node";
}
