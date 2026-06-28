# Dead Code Elimination (DeadCodeEliminator)

Los pases anteriores reescriben **expresiones**: `2 + 3` → `5`, `x + 0` → `x`. El
último pase opera un nivel más arriba, sobre **sentencias enteras**: descarta el
código que nunca se ejecuta o cuya rama está decidida de antemano.

```cpp
int f() {
    return 1;
    print(2);        // inalcanzable: nunca se ejecuta tras el return
}
if (true) { a(); } else { b(); }   // la rama 'else' está muerta
while (false) { ... }              // el cuerpo nunca corre
```

Lo implementa `DeadCodeEliminator` (en `dead_code_eliminator.h` / `.cpp`), que solo
redefine `visit(Block)`. Es el pase que más se beneficia de correr **al final**:
para cuando llega, el `ConstantFolder` ya convirtió condiciones como `2 > 1` en
literales `true`, dándole material concreto que eliminar.

---

## 1. Por qué `visit(Block)`

Las sentencias no son nodos sueltos reapuntables como las expresiones: siempre
viven en la lista `std::vector<Stmt*>` de un `Block`. Por eso el mecanismo
`walk`/`replaceWith` de `AstWalker` —pensado para slots de expresión— no aplica
aquí. En su lugar, el pase **reconstruye el vector de sentencias** del bloque,
quedándose solo con lo que sobrevive:

```cpp
void DeadCodeEliminator::visit(Block* node) {
    AstWalker::visit(node);   // limpia las sentencias hijas (bloques anidados) primero

    std::vector<Stmt*> kept;
    bool terminated = false;
    for (Stmt* s : node->stmts) {
        // ... decidir qué hacer con cada s ...
    }
    node->stmts = std::move(kept);
}
```

`AstWalker::visit(node)` primero recorre y limpia los bloques anidados (un `if`
dentro del `if`, etc.); luego se procesa la lista de este nivel. Lo que se
conserva va a `kept`; lo que se elimina se libera con `delete`.

---

## 2. Código inalcanzable tras un terminador

La primera forma de código muerto: todo lo que sigue a un `return`, `break` o
`continue` dentro del mismo bloque nunca se ejecuta.

```cpp
static bool isTerminator(Stmt* s) {
    return dynamic_cast<ReturnStmt*>(s) || dynamic_cast<BreakStmt*>(s) ||
           dynamic_cast<ContinueStmt*>(s);
}
```

El flag `terminated` se levanta al ver el primer terminador, y a partir de ahí
todo se descarta:

```cpp
for (Stmt* s : node->stmts) {
    if (terminated) { delete s; continue; }   // inalcanzable
    ...
    kept.push_back(s);
    if (isTerminator(s)) terminated = true;    // a partir de aquí, muerto
}
```

---

## 3. `if` con condición constante

Si la condición de un `if` ya es un literal (gracias al folding previo), solo una
rama puede ejecutarse; la otra es código muerto. El pase **reemplaza el `if` por la
rama que se toma**:

```cpp
if (auto* iff = dynamic_cast<IfStmt*>(s)) {
    bool t;
    if (litTruth(iff->condition, t)) {
        if (t) {
            kept.push_back(iff->then_branch);   // rescata la rama 'then'
            iff->then_branch = nullptr;         // ...y la desengancha
        } else if (iff->else_branch) {
            kept.push_back(iff->else_branch);
            iff->else_branch = nullptr;
        }
        delete iff;   // libera la condición y la rama NO tomada
        continue;
    }
}
```

`litTruth` extrae el valor de verdad de un literal (`bool`, `int` o `float`; `0` es
falso, lo demás verdadero). El patrón es el mismo `detach` del
`AlgebraicSimplifier` (`4_algebraic_simplification.md §3`): la rama que se conserva
se pone a `nullptr` en el `IfStmt` **antes** de borrarlo, para que el destructor del
`if` no la arrastre — solo libere la condición y la rama descartada.

`if (true) { a } else { b }` queda como `{ a }` insertado directo en el bloque
padre. Si la condición no es literal, el `if` se conserva tal cual.

---

## 4. `while (false)`

Un bucle cuya condición es un literal falso nunca ejecuta su cuerpo, así que se
descarta entero:

```cpp
if (auto* wh = dynamic_cast<WhileStmt*>(s)) {
    bool t;
    if (litTruth(wh->condition, t) && !t) { delete wh; continue; }
}
```

Nótese la asimetría con el `if`: solo se elimina `while (false)`. Un
`while (true)` **no** se toca, porque su cuerpo sí corre (es un bucle infinito,
posiblemente intencional con un `break` adentro) — eliminarlo cambiaría el
comportamiento.

---

## En contexto

El `DeadCodeEliminator` cierra la cadena de pases (`1_optimizer.md §1`) y depende
por completo de los anteriores: sin el `ConstantFolder`
(`2_constant_folding.md`) que convierte las condiciones en literales, casi no
tendría nada que eliminar. Es el ejemplo más claro de por qué el **orden de los
pases importa** y por qué cada uno habilita al siguiente. Tras él, el AST optimizado
pasa al `CodeGenerator` (`5_codegen/1_code_generator.md`), que lo traduce a
ensamblador sin enterarse de que pasó por el optimizador.
