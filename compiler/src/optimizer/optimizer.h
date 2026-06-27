#pragma once

class Program;

namespace optimizer {

// Aplica las optimizaciones sobre el AST in-place, antes del codegen.
// Por ahora es un no-op: aquí se engancharán los pases (constant folding,
// dead code elimination, ...). El flag `--opt` de main.cpp decide si se invoca.
void optimize(Program* program);

}  // namespace optimizer
