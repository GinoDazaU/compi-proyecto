import { useEffect, useRef, useState } from "react";
import Tree from "react-d3-tree";
import { astToTree } from "../astTree";

export default function AstTree({ ast }) {
  const containerRef = useRef(null);
  const [translate, setTranslate] = useState({ x: 0, y: 0 });
  const data = astToTree(ast);

  // Centra el árbol horizontalmente al montar o cambiar el AST.
  useEffect(() => {
    if (containerRef.current) {
      const { width } = containerRef.current.getBoundingClientRect();
      setTranslate({ x: width / 2, y: 56 });
    }
  }, [ast]);

  return (
    <div ref={containerRef} className="h-full w-full bg-stone-50">
      <Tree
        data={data}
        orientation="vertical"
        translate={translate}
        pathFunc="step"
        nodeSize={{ x: 170, y: 90 }}
        separation={{ siblings: 1.1, nonSiblings: 1.4 }}
        zoom={0.8}
        scaleExtent={{ min: 0.2, max: 2.5 }}
        collapsible={false}
        renderCustomNodeElement={renderNode}
      />
    </div>
  );
}

// Cada nodo se dibuja como una cajita con el tipo y sus campos primitivos.
function renderNode({ nodeDatum }) {
  const attrs = Object.entries(nodeDatum.attributes || {});
  const height = 30 + attrs.length * 15;

  return (
    <g>
      <foreignObject x={-75} y={-height / 2} width={150} height={height}>
        <div className="flex h-full flex-col justify-center rounded-md border border-stone-300 bg-white px-2 py-1 shadow-sm">
          <div className="truncate text-center text-[11px] font-semibold text-green-700">
            {nodeDatum.name}
          </div>
          {attrs.map(([k, v]) => (
            <div key={k} className="truncate text-center text-[10px] text-stone-500">
              {k}: <span className="text-stone-700">{String(v)}</span>
            </div>
          ))}
        </div>
      </foreignObject>
    </g>
  );
}
