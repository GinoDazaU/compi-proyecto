#include "constant_folder.h"
#include "opt_util.h"
#include "../parser/ast.h"

using namespace optutil;

// Patrón: primero plegar los hijos (AstWalker::visit recurre), luego intentar
// plegar este nodo. Si los operandos quedaron literales, calcular el valor y
// reemplazar el nodo con un literal nuevo vía replaceWith(...).

void ConstantFolder::visit(BinaryExpr* node) {
    AstWalker::visit(node);  // pliega left/right primero

    double ld, rd;
    if (!asDouble(node->left, ld) || !asDouble(node->right, rd)) return;

    long long li, ri;
    bool bothInt = asLong(node->left, li) && asLong(node->right, ri);

    switch (node->op) {
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:
            if (bothInt) {
                if ((node->op == BinaryOp::Div || node->op == BinaryOp::Mod) && ri == 0) return;
                long long r = 0;
                switch (node->op) {
                    case BinaryOp::Add: r = li + ri; break;
                    case BinaryOp::Sub: r = li - ri; break;
                    case BinaryOp::Mul: r = li * ri; break;
                    case BinaryOp::Div: r = li / ri; break;
                    default:            r = li % ri; break;
                }
                replaceWith(new IntLitExpr(r));
            } else {
                double r = 0;
                switch (node->op) {
                    case BinaryOp::Add: r = ld + rd; break;
                    case BinaryOp::Sub: r = ld - rd; break;
                    case BinaryOp::Mul: r = ld * rd; break;
                    case BinaryOp::Div: if (rd == 0) return; r = ld / rd; break;
                    default: return;  // % no aplica a float
                }
                replaceWith(new FloatLitExpr(r));
            }
            break;

        case BinaryOp::Eq:  replaceWith(new BoolLitExpr(ld == rd)); break;
        case BinaryOp::Neq: replaceWith(new BoolLitExpr(ld != rd)); break;
        case BinaryOp::Lt:  replaceWith(new BoolLitExpr(ld <  rd)); break;
        case BinaryOp::Gt:  replaceWith(new BoolLitExpr(ld >  rd)); break;
        case BinaryOp::Leq: replaceWith(new BoolLitExpr(ld <= rd)); break;
        case BinaryOp::Geq: replaceWith(new BoolLitExpr(ld >= rd)); break;
        case BinaryOp::And: replaceWith(new BoolLitExpr(ld != 0 && rd != 0)); break;
        case BinaryOp::Or:  replaceWith(new BoolLitExpr(ld != 0 || rd != 0)); break;
    }
}

void ConstantFolder::visit(UnaryExpr* node) {
    AstWalker::visit(node);  // pliega el operando primero

    if (node->op == UnaryOp::Neg) {
        long long li;
        double ld;
        if (asLong(node->expr, li))      replaceWith(new IntLitExpr(-li));
        else if (isFloatLit(node->expr) && asDouble(node->expr, ld))
                                         replaceWith(new FloatLitExpr(-ld));
    } else if (node->op == UnaryOp::Not) {
        double d;
        if (asDouble(node->expr, d)) replaceWith(new BoolLitExpr(d == 0));
    }
}
