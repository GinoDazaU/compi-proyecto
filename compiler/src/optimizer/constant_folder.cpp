#include "constant_folder.h"
#include "../parser/ast.h"

// Patrón: primero plegar los hijos (AstWalker::visit recurre), luego intentar
// plegar este nodo. Si ambos operandos quedaron literales, calcular el valor y
// reemplazar el nodo con un literal nuevo vía replaceWith(...).

void ConstantFolder::visit(BinaryExpr* node) {
    AstWalker::visit(node);  // pliega left/right primero
    // TODO: si node->left y node->right son literales numéricos, evaluar
    // node->op y replaceWith(new IntLitExpr(...) / new FloatLitExpr(...)).
}

void ConstantFolder::visit(UnaryExpr* node) {
    AstWalker::visit(node);  // pliega el operando primero
    // TODO: plegar -lit, !lit, etc. sobre operando literal.
}
