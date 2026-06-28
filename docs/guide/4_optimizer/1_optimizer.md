# El Optimizador

El **Optimizador** es una fase **opcional** que corre entre el semántico y el
codegen, solo cuando se pasa el flag `--opt`. Recibe el AST ya validado y lo
**reescribe in-place**: produce otro AST equivalente —que hace exactamente lo
mismo— pero con menos trabajo en tiempo de ejecución. No genera código ni cambia
representación; entra un AST y sale el mismo AST, mejorado.

```cpp
// main.cpp, Fase 3.5
if (opt) optimizer::optimize(program);
```

A diferencia del resto de la guía, el optimizador no es una sola clase grande:
es un **conjunto de pases pequeños**, cada uno en su propio archivo, que se
encadenan. Este documento explica la arquitectura común —el *pass manager* y el
recorrido reescribible (`AstWalker`)— y los siguientes detallan cada pase:

- `2_constant_folding.md` — plegar operaciones entre literales (`2 + 3` → `5`).
- `3_constant_propagation.md` — sustituir variables constantes por su literal.
- `4_algebraic_simplification.md` — identidades como `x + 0` → `x`.
- `5_dead_code.md` — descartar ramas y código inalcanzable.

---

## 1. El pass manager: `optimize`

El punto de entrada vive en `optimizer.cpp` y es deliberadamente corto: ejecuta
los pases, uno tras otro, sobre el mismo árbol.

```cpp
void optimize(Program* program) {
    ConstantFolder().run(program);       // pliega literales (incluye los inits)
    ConstantPropagator().run(program);   // sustituye variables constantes por su literal
    ConstantFolder().run(program);       // pliega lo recién expuesto por la propagación
    AlgebraicSimplifier().run(program);  // x+0, 0+x, x-0, x*1, 1*x, x/1
    DeadCodeEliminator().run(program);   // if/while con condición constante, código inalcanzable
}
```

Cada pase es un objeto independiente que recorre el AST y lo modifica. Lo
interesante es que **el orden importa**, porque un pase habilita al siguiente:

- Propagar constantes (`x = 5; ... x + 1`) deja literales nuevos donde antes
  había variables, así que conviene **volver a plegar** después: por eso
  `ConstantFolder` aparece **dos veces**, antes y después de la propagación.
- Plegar condiciones (`if (2 > 1)` → `if (true)`) expone ramas que nunca se
  ejecutan, así que el `DeadCodeEliminator` corre **al final**, cuando esas
  condiciones ya son literales y puede decidir qué rama eliminar.

Esta es la idea clave del diseño: pases pequeños, cada uno hace **una** cosa
bien, y la potencia surge de **encadenarlos**. Agregar una optimización nueva es
incluir su header y añadir una línea aquí.

---

## 2. El problema del recorrido reescribible

Todos los pases necesitan lo mismo: **recorrer el AST y poder reemplazar nodos**.
Pero el patrón Visitor que usa todo el compilador tiene una limitación incómoda
para esto: `visit(...)` devuelve `void`. Un `visit(BinaryExpr)` que descubre que
`2 + 3` es `5` no puede *retornar* el nuevo `IntLitExpr(5)` para que ocupe su
lugar — no tiene a quién devolvérselo.

El semántico y el codegen sortearon esto dejando el resultado en un miembro
(`expr_type_`, `cur_type_`). El optimizador necesita algo más fuerte: no basta
con *calcular* algo sobre el nodo, hay que **sustituir el nodo entero** en el
árbol, lo que significa modificar el puntero que lo guarda **en su padre**.

La solución es la clase base `AstWalker`.

---

## 3. `AstWalker`: el recorrido por defecto

`AstWalker` (en `ast_walker.h` / `.cpp`) hereda de `Visitor` e implementa **todas**
las `visit` con el comportamiento más simple posible: descender a los hijos sin
hacer nada más. Es un recorrido completo y neutro del árbol.

```cpp
void AstWalker::visit(BinaryExpr* node) { walk(node->left); walk(node->right); }
void AstWalker::visit(IfStmt* node) {
    walk(node->condition);
    node->then_branch->accept(this);
    if (node->else_branch) node->else_branch->accept(this);
}
// ... y así para cada nodo, descendiendo a cada hijo
```

Sobre esta base, **cada pase solo sobreescribe los nodos que le interesan** y
llama a `AstWalker::visit(node)` para conservar el descenso a los hijos. El
`ConstantFolder`, por ejemplo, solo redefine `visit(BinaryExpr)` y
`visit(UnaryExpr)`; todo lo demás lo hereda tal cual. Esto evita que cada pase
tenga que reimplementar el recorrido de los ~30 tipos de nodo.

---

## 4. El mecanismo de reemplazo: `walk` + `replaceWith`

Aquí está el corazón del diseño. Dos piezas trabajan juntas:

```cpp
protected:
    void replaceWith(Expr* e) { repl_ = e; }   // un pase la llama en su visit
    void walk(Expr*& slot);                    // visita un slot y aplica el reemplazo
private:
    Expr* repl_ = nullptr;                      // reemplazo pendiente
```

Cuando un pase quiere sustituir la expresión actual, llama a `replaceWith(nuevo)`,
que solo **anota** el nodo de reemplazo en el miembro `repl_`. No toca el árbol
todavía: en ese momento el `visit` no tiene acceso al puntero del padre.

Quien sí lo tiene es `walk`, que recibe el slot **por referencia** (`Expr*&` — una
referencia al puntero que el padre guarda) y es responsable de aplicar el cambio:

```cpp
void AstWalker::walk(Expr*& slot) {
    if (!slot) return;
    Expr* saved = repl_;
    repl_ = nullptr;
    slot->accept(this);            // el pase puede llamar replaceWith durante esto
    if (repl_ && repl_ != slot) {
        delete slot;               // libera el nodo viejo (y su subárbol)
        slot = repl_;              // el padre ahora apunta al nuevo
    }
    repl_ = saved;
}
```

El flujo completo, paso a paso:

1. El padre llama `walk(node->left)` — pasa una referencia a su propio puntero.
2. `walk` invoca `slot->accept(this)`, que despacha al `visit` del pase.
3. Si el pase decide reemplazar, llama `replaceWith(nuevo)`, que deja `repl_ = nuevo`.
4. De vuelta en `walk`, ve que `repl_` no es nulo: **borra el nodo viejo** y
   **reapunta el slot del padre** al nuevo.

Así, aunque `visit` devuelva `void`, el reemplazo llega a su destino: el puntero
del padre. Es el equivalente a "devolver el nodo reescrito desde `accept()`", pero
hecho a través de un miembro y una referencia al slot.

> **Guardar y restaurar `repl_`.** Fíjate en `saved`: antes de descender, `walk`
> guarda el `repl_` actual y lo restaura al volver. Esto es necesario porque el
> recorrido es recursivo —un `walk` puede ocurrir dentro de otro— y sin salvar el
> estado, el reemplazo de un nivel interno se confundiría con el del externo.

---

## 5. Reescribir sentencias, no solo expresiones

El mecanismo `walk`/`replaceWith` resuelve el reemplazo de **expresiones**, que es
el caso común (un `BinaryExpr` se vuelve un `IntLitExpr`). Pero algunos pases
necesitan reescribir **sentencias**: el `DeadCodeEliminator` elimina sentencias
enteras de un bloque, y el `if` con condición constante se reduce a la rama tomada.

Para eso no hace falta maquinaria extra: una sentencia siempre vive dentro de un
`Block`, en un `std::vector<Stmt*>`. El pase sobreescribe `visit(Block)` y
**reconstruye el vector** filtrando lo que sobra (ver `5_dead_code.md`). Borrar un
elemento de un vector es directo; lo que `walk` resuelve —reapuntar un puntero
suelto en el padre— solo hacía falta para las expresiones.

---

## 6. Seguridad: por qué los pases son conservadores

Una optimización **nunca** puede cambiar el comportamiento del programa. Cada
pase está escrito para rendirse (no optimizar) ante la menor duda:

- El `ConstantFolder` no pliega una división entre cero: la deja como está para
  que el comportamiento en runtime se preserve.
- El `ConstantPropagator` no propaga una variable cuya dirección se toma (`&x`),
  porque podría mutarse a través del puntero sin que se vea en el flujo.
- El `AlgebraicSimplifier` solo aplica identidades donde el operando descartado es
  el literal `0`/`1` —sin efectos secundarios que perder.

La regla de oro: **ante la duda, no optimizar**. Un AST sin optimizar siempre es
correcto; uno mal optimizado, no.

---

## En contexto

El optimizador es la única fase que **transforma** el AST en lugar de solo
recorrerlo (semántico) o traducirlo (codegen). Se apoya en el mismo patrón Visitor
que el resto de la guía, extendido con `AstWalker` para permitir la reescritura. El
AST que sale de aquí entra al `CodeGenerator` (`5_codegen/1_code_generator.md`)
exactamente igual que uno sin optimizar — el codegen no sabe ni le importa si pasó
por el optimizador. Los pases concretos se explican en los documentos siguientes.
