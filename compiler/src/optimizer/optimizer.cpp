#include "optimizer.h"
#include "constant_folder.h"
#include "dead_code_eliminator.h"

namespace optimizer {

// Pass manager: cada optimización es un AstWalker que se corre en orden sobre el
// mismo AST. El orden importa (plegar constantes antes expone más ramas muertas).
// Agregar un pase = incluir su header y añadir una línea aquí.
void optimize(Program* program) {
    ConstantFolder().run(program);
    DeadCodeEliminator().run(program);
}

}  // namespace optimizer
