#include "dead_code_eliminator.h"
#include "../parser/ast.h"

void DeadCodeEliminator::visit(Block* node) {
    AstWalker::visit(node);  // recurre en las sentencias hijas primero
    // TODO: filtrar node->stmts — descartar lo que sigue a un return/break/
    // continue, y plegar ifs/while con condición constante. Recordar liberar
    // (delete) las sentencias eliminadas.
}
