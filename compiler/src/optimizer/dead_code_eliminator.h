#pragma once
#include "ast_walker.h"

// ─── DeadCodeEliminator ──────────────────────────────────────────────────────
// Descarta código inalcanzable o sin efecto (p.ej. sentencias tras un return,
// ramas de un if con condición constante). Hereda el recorrido de AstWalker y
// reescribe la lista de sentencias de cada bloque.
class DeadCodeEliminator : public AstWalker {
public:
    void visit(Block* node) override;
};
