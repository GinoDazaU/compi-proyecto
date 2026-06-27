#pragma once
#include "ast_walker.h"

// ─── ConstantFolder ──────────────────────────────────────────────────────────
// Plega expresiones con operandos literales en tiempo de compilación
// (p.ej. 2 + 3 → 5, !true → false). Hereda el recorrido de AstWalker y solo
// sobreescribe los nodos que puede plegar.
class ConstantFolder : public AstWalker {
public:
    void visit(BinaryExpr* node) override;
    void visit(UnaryExpr* node) override;
};
