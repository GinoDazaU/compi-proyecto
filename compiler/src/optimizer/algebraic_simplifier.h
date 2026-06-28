#pragma once
#include "ast_walker.h"

// ─── AlgebraicSimplifier ─────────────────────────────────────────────────────
// Aplica identidades algebraicas seguras que devuelven uno de los operandos sin
// crear literales nuevos (no cambian el tipo): x+0, 0+x, x-0, x*1, 1*x, x/1.
// El operando descartado siempre es el literal 0/1, así que no hay efectos que
// perder.
class AlgebraicSimplifier : public AstWalker {
public:
    void visit(BinaryExpr* node) override;
};
