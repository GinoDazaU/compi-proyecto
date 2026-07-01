#include "algebraic_simplifier.h"
#include "opt_util.h"
#include "../parser/ast.h"

using namespace optutil;

// Desengancha un operando del nodo (lo pone a nullptr) para conservarlo como
// reemplazo sin que el destructor del BinaryExpr lo libere.
static Expr* detach(Expr*& slot) {
    Expr* e = slot;
    slot = nullptr;
    return e;
}

// Una identidad float (`* 1.0`, `+ 0.0`) promueve el otro operando a float;
// quitarla si el sobreviviente no es float cambiaría el tipo (y la semántica:
// la división pasaría a entera). Sin tipos en el AST, exigimos que ambos lo sean.
static bool safeToDrop(Expr* identity, Expr* survivor) {
    return !isFloatLit(identity) || isFloatLit(survivor);
}

void AlgebraicSimplifier::visit(BinaryExpr* node) {
    AstWalker::visit(node);  // simplifica los hijos primero

    Expr* L = node->left;
    Expr* R = node->right;

    switch (node->op) {
        case BinaryOp::Add:
            if      (isZero(R) && safeToDrop(R, L)) replaceWith(detach(node->left));   // x + 0 → x
            else if (isZero(L) && safeToDrop(L, R)) replaceWith(detach(node->right));  // 0 + x → x
            break;
        case BinaryOp::Sub:
            if (isZero(R) && safeToDrop(R, L)) replaceWith(detach(node->left));        // x - 0 → x
            break;
        case BinaryOp::Mul:
            if      (isOne(R) && safeToDrop(R, L)) replaceWith(detach(node->left));    // x * 1 → x
            else if (isOne(L) && safeToDrop(L, R)) replaceWith(detach(node->right));   // 1 * x → x
            break;
        case BinaryOp::Div:
            if (isOne(R) && safeToDrop(R, L)) replaceWith(detach(node->left));         // x / 1 → x
            break;
        default:
            break;
    }
}
