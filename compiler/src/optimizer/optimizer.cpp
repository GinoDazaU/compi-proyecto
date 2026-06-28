#include "optimizer.h"
#include "constant_folder.h"
#include "constant_propagator.h"
#include "algebraic_simplifier.h"
#include "dead_code_eliminator.h"

namespace optimizer {

// Pass manager: cada optimización es un AstWalker que se corre en orden sobre el
// mismo AST. El orden importa: propagar constantes expone más literales a plegar,
// y plegar expone más ramas muertas. Agregar un pase = incluir su header y una línea.
void optimize(Program* program) {
    ConstantFolder().run(program);       // pliega literales (incluye los inits)
    ConstantPropagator().run(program);   // sustituye variables constantes por su literal
    ConstantFolder().run(program);       // pliega lo recién expuesto por la propagación
    AlgebraicSimplifier().run(program);  // x+0, 0+x, x-0, x*1, 1*x, x/1
    DeadCodeEliminator().run(program);   // if/while con condición constante, código inalcanzable
}

}  // namespace optimizer
