#include "dead_code_eliminator.h"
#include "../parser/ast.h"
#include <vector>

// ¿La expresión es un literal cuyo valor de verdad conocemos? (corre tras el
// ConstantFolder, así que las condiciones constantes ya son literales)
static bool litTruth(Expr* e, bool& truth) {
    if (auto* b = dynamic_cast<BoolLitExpr*>(e))   { truth = b->value;      return true; }
    if (auto* i = dynamic_cast<IntLitExpr*>(e))    { truth = i->value != 0; return true; }
    if (auto* f = dynamic_cast<FloatLitExpr*>(e))  { truth = f->value != 0; return true; }
    return false;
}

static bool isTerminator(Stmt* s) {
    return dynamic_cast<ReturnStmt*>(s) || dynamic_cast<BreakStmt*>(s) ||
           dynamic_cast<ContinueStmt*>(s);
}

void DeadCodeEliminator::visit(Block* node) {
    AstWalker::visit(node);  // limpia las sentencias hijas primero

    std::vector<Stmt*> kept;
    bool terminated = false;

    for (Stmt* s : node->stmts) {
        if (terminated) { delete s; continue; }  // inalcanzable tras un terminador

        // if con condición constante → se queda solo la rama que se toma.
        if (auto* iff = dynamic_cast<IfStmt*>(s)) {
            bool t;
            if (litTruth(iff->condition, t)) {
                if (t) {
                    kept.push_back(iff->then_branch);
                    iff->then_branch = nullptr;
                } else if (iff->else_branch) {
                    kept.push_back(iff->else_branch);
                    iff->else_branch = nullptr;
                }
                delete iff;  // libera la condición y la rama no tomada
                continue;
            }
        }

        // while(false) nunca ejecuta → se descarta.
        if (auto* wh = dynamic_cast<WhileStmt*>(s)) {
            bool t;
            if (litTruth(wh->condition, t) && !t) { delete wh; continue; }
        }

        kept.push_back(s);
        if (isTerminator(s)) terminated = true;
    }

    node->stmts = std::move(kept);
}
