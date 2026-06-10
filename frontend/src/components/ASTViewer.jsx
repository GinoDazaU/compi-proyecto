import { useRef, useEffect, useState } from 'react';
import * as d3 from 'd3';

function astToD3(node) {
  if (!node || typeof node !== 'object') return null;

  const type = node.type || 'Unknown';
  let label = type;
  const children = [];

  switch (type) {
    case 'Program':
      label = 'Program';
      (node.decls || []).forEach(d => {
        const c = astToD3(d);
        if (c) children.push(c);
      });
      break;

    case 'FuncDecl':
      label = `FuncDecl: ${node.name}`;
      if (node.params?.length) {
        children.push({
          name: 'Params',
          _nodeType: 'meta',
          children: node.params.map(p => ({
            name: `${p.name}: ${p.type || '?'}`,
            _nodeType: 'param',
          })),
        });
      }
      if (node.body) {
        const b = astToD3(node.body);
        if (b) children.push(b);
      }
      break;

    case 'TemplateFuncDecl':
      label = `Template<${node.template_param}>`;
      if (node.func) {
        const f = astToD3(node.func);
        if (f) children.push(f);
      }
      break;

    case 'StructDecl':
      label = `Struct: ${node.name}`;
      (node.members || []).forEach(m => {
        children.push({ name: `${m.name}: ${m.type || '?'}`, _nodeType: 'field' });
      });
      break;

    case 'GlobalVarDecl':
      label = `Global: ${node.name}`;
      if (node.init) { const c = astToD3(node.init); if (c) children.push(c); }
      break;

    case 'Block':
      label = 'Block';
      (node.stmts || []).forEach(s => {
        const c = astToD3(s);
        if (c) children.push(c);
      });
      break;

    case 'VarDeclStmt':
      label = `Var: ${node.name}`;
      if (node.init) { const c = astToD3(node.init); if (c) children.push(c); }
      (node.init_list || []).forEach(e => { const c = astToD3(e); if (c) children.push(c); });
      break;

    case 'ExprStmt':
      label = 'ExprStmt';
      if (node.expr) { const c = astToD3(node.expr); if (c) children.push(c); }
      break;

    case 'IfStmt':
      label = 'If';
      if (node.condition) children.push({ ...astToD3(node.condition), _role: 'cond' });
      if (node.then_branch) children.push({ ...astToD3(node.then_branch), _role: 'then' });
      if (node.else_branch) children.push({ ...astToD3(node.else_branch), _role: 'else' });
      break;

    case 'WhileStmt':
      label = 'While';
      if (node.condition) { const c = astToD3(node.condition); if (c) children.push(c); }
      if (node.body) { const c = astToD3(node.body); if (c) children.push(c); }
      break;

    case 'ForStmt':
      label = 'For';
      if (node.init) { const c = astToD3(node.init); if (c) children.push(c); }
      if (node.condition) { const c = astToD3(node.condition); if (c) children.push(c); }
      if (node.update) { const c = astToD3(node.update); if (c) children.push(c); }
      if (node.body) { const c = astToD3(node.body); if (c) children.push(c); }
      break;

    case 'ForRangeStmt':
      label = `ForRange: ${node.name}`;
      if (node.iterable) { const c = astToD3(node.iterable); if (c) children.push(c); }
      if (node.body) { const c = astToD3(node.body); if (c) children.push(c); }
      break;

    case 'ReturnStmt':
      label = 'Return';
      if (node.expr) { const c = astToD3(node.expr); if (c) children.push(c); }
      break;

    case 'BreakStmt': label = 'Break'; break;
    case 'ContinueStmt': label = 'Continue'; break;

    case 'DeleteStmt':
      label = node.is_array ? 'Delete[]' : 'Delete';
      if (node.expr) { const c = astToD3(node.expr); if (c) children.push(c); }
      break;

    case 'BinaryExpr':
      label = `BinOp: ${node.op}`;
      if (node.left) { const c = astToD3(node.left); if (c) children.push(c); }
      if (node.right) { const c = astToD3(node.right); if (c) children.push(c); }
      break;

    case 'UnaryExpr':
      label = `Unary: ${node.op}`;
      if (node.expr) { const c = astToD3(node.expr); if (c) children.push(c); }
      break;

    case 'AssignExpr':
      label = `Assign: ${node.op}`;
      if (node.left) { const c = astToD3(node.left); if (c) children.push(c); }
      if (node.right) { const c = astToD3(node.right); if (c) children.push(c); }
      break;

    case 'CallExpr':
      label = 'Call';
      if (node.callee) { const c = astToD3(node.callee); if (c) children.push(c); }
      (node.args || []).forEach(a => { const c = astToD3(a); if (c) children.push(c); });
      break;

    case 'IndexExpr':
      label = 'Index';
      if (node.base) { const c = astToD3(node.base); if (c) children.push(c); }
      if (node.index) { const c = astToD3(node.index); if (c) children.push(c); }
      break;

    case 'MemberExpr':
      label = `${node.is_arrow ? '->' : '.'}${node.member}`;
      if (node.base) { const c = astToD3(node.base); if (c) children.push(c); }
      break;

    case 'PostfixExpr':
      label = `Postfix: ${node.is_inc ? '++' : '--'}`;
      if (node.base) { const c = astToD3(node.base); if (c) children.push(c); }
      break;

    case 'CastExpr':
      label = `Cast→${node.targetType || '?'}`;
      if (node.expr) { const c = astToD3(node.expr); if (c) children.push(c); }
      break;

    case 'NewArrayExpr':
      label = `New[${node.elementType || '?'}]`;
      if (node.size) { const c = astToD3(node.size); if (c) children.push(c); }
      break;

    case 'NewObjectExpr':
      label = `New ${node.objectType || '?'}`;
      (node.args || []).forEach(a => { const c = astToD3(a); if (c) children.push(c); });
      break;

    case 'IntLitExpr': label = `${node.value}`; break;
    case 'FloatLitExpr': label = `${node.value}`; break;
    case 'BoolLitExpr': label = `${node.value}`; break;
    case 'CharLitExpr': label = `'${node.value}'`; break;
    case 'StringLitExpr': label = `"${node.value}"`; break;
    case 'IdExpr': label = node.name; break;

    case 'LambdaExpr':
      label = 'Lambda';
      if (node.body) { const c = astToD3(node.body); if (c) children.push(c); }
      break;

    default:
      label = type;
  }

  const result = { name: label, _nodeType: nodeCategory(type) };
  if (children.length > 0) result.children = children;
  return result;
}

function nodeCategory(type) {
  if (['IntLitExpr','FloatLitExpr','BoolLitExpr','CharLitExpr','StringLitExpr'].includes(type)) return 'literal';
  if (['IdExpr'].includes(type)) return 'identifier';
  if (['BinaryExpr','UnaryExpr','AssignExpr','CastExpr','PostfixExpr'].includes(type)) return 'operator';
  if (['CallExpr','IndexExpr','MemberExpr','NewArrayExpr','NewObjectExpr'].includes(type)) return 'postfix';
  if (['IfStmt','WhileStmt','ForStmt','ForRangeStmt','ReturnStmt','BreakStmt','ContinueStmt','DeleteStmt','ExprStmt'].includes(type)) return 'statement';
  if (['FuncDecl','StructDecl','TemplateFuncDecl','GlobalVarDecl','VarDeclStmt'].includes(type)) return 'declaration';
  if (['Block','Program'].includes(type)) return 'structure';
  return 'other';
}

const NODE_COLORS = {
  literal: '#fbbf24',
  identifier: '#e2e8f0',
  operator: '#67e8f9',
  postfix: '#a78bfa',
  statement: '#f472b6',
  declaration: '#34d399',
  structure: '#6366f1',
  param: '#94a3b8',
  field: '#94a3b8',
  meta: '#64748b',
  other: '#94a3b8',
};

export default function ASTViewer({ ast }) {
  const svgRef = useRef(null);
  const containerRef = useRef(null);
  const [tooltip, setTooltip] = useState(null);

  useEffect(() => {
    if (!ast || !svgRef.current || !containerRef.current) return;

    const d3Data = astToD3(ast);
    if (!d3Data) return;

    const svg = d3.select(svgRef.current);
    svg.selectAll('*').remove();

    const container = containerRef.current;
    const width = container.clientWidth;
    const height = container.clientHeight;

    const root = d3.hierarchy(d3Data);
    const nodeCount = root.descendants().length;

    // Dynamic sizing based on tree complexity
    const treeWidth = Math.max(width - 80, nodeCount * 25);
    const treeHeight = Math.max(height - 80, root.height * 80);

    const treeLayout = d3.tree().size([treeWidth, treeHeight]);
    treeLayout(root);

    // Setup zoom
    const g = svg.append('g');
    const zoom = d3.zoom()
      .scaleExtent([0.1, 3])
      .on('zoom', (event) => g.attr('transform', event.transform));

    svg.call(zoom);

    // Center the tree initially
    const initialScale = Math.min(1, width / (treeWidth + 100), height / (treeHeight + 100));
    const tx = (width - treeWidth * initialScale) / 2;
    const ty = 40;
    svg.call(zoom.transform, d3.zoomIdentity.translate(tx, ty).scale(initialScale));

    // Draw links
    g.selectAll('.ast-link')
      .data(root.links())
      .join('path')
      .attr('class', 'ast-link')
      .attr('d', d3.linkVertical().x(d => d.x).y(d => d.y));

    // Draw nodes
    const nodes = g.selectAll('.ast-node')
      .data(root.descendants())
      .join('g')
      .attr('class', 'ast-node')
      .attr('transform', d => `translate(${d.x},${d.y})`);

    nodes.append('circle')
      .attr('r', d => d.children ? 6 : 4)
      .attr('fill', d => NODE_COLORS[d.data._nodeType] || NODE_COLORS.other)
      .attr('stroke', d => {
        const color = NODE_COLORS[d.data._nodeType] || NODE_COLORS.other;
        return d3.color(color).brighter(0.5);
      })
      .attr('stroke-width', 1.5)
      .on('mouseover', (event, d) => {
        const [mx, my] = d3.pointer(event, container);
        setTooltip({ x: mx + 10, y: my - 10, text: d.data.name, type: d.data._nodeType });
        d3.select(event.target)
          .transition().duration(150)
          .attr('r', d.children ? 9 : 7)
          .attr('filter', 'drop-shadow(0 0 6px rgba(99,102,241,0.5))');
      })
      .on('mouseout', (event, d) => {
        setTooltip(null);
        d3.select(event.target)
          .transition().duration(150)
          .attr('r', d.children ? 6 : 4)
          .attr('filter', null);
      });

    // Labels
    nodes.append('text')
      .attr('dy', d => d.children ? -12 : 16)
      .attr('text-anchor', 'middle')
      .text(d => {
        const name = d.data.name;
        return name.length > 18 ? name.slice(0, 16) + '…' : name;
      })
      .style('fill', d => NODE_COLORS[d.data._nodeType] || NODE_COLORS.other)
      .style('font-size', '10px');

  }, [ast]);

  if (!ast) {
    return (
      <div className="empty-state">
        <div className="empty-state__icon">🌳</div>
        <div className="empty-state__text">
          Compilá tu código para ver el AST
        </div>
      </div>
    );
  }

  return (
    <div ref={containerRef} className="ast-container">
      <svg ref={svgRef} />
      {tooltip && (
        <div className="ast-tooltip" style={{ left: tooltip.x, top: tooltip.y }}>
          <strong style={{ color: NODE_COLORS[tooltip.type] }}>{tooltip.type}</strong>
          <br />
          {tooltip.text}
        </div>
      )}
    </div>
  );
}
