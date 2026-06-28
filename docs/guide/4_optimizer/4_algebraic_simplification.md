# Simplificación Algebraica (AlgebraicSimplifier)

El *constant folding* (`2_constant_folding.md`) resuelve operaciones donde **ambos**
operandos son literales. Pero hay un caso intermedio muy común que no puede tocar:
una operación entre una **variable** y un literal "neutro".

```cpp
int y = x + 0;   // no se puede plegar: 'x' no es literal
int z = w * 1;   // ...pero el resultado es, obviamente, 'w'
```

La **simplificación algebraica** aplica identidades matemáticas conocidas para
eliminar estas operaciones inútiles, reemplazando el nodo entero por el operando
que de verdad importa. Lo implementa `AlgebraicSimplifier` (en
`algebraic_simplifier.h` / `.cpp`), que solo redefine `visit(BinaryExpr)`.

---

## 1. Las identidades soportadas

El pase aplica seis identidades, todas con el literal `0` o `1` como operando
neutro:

| Expresión | Se simplifica a |
|---|---|
| `x + 0`, `0 + x` | `x` |
| `x - 0` | `x` |
| `x * 1`, `1 * x` | `x` |
| `x / 1` | `x` |

Nótese qué **no** está: `x - x` → `0`, `x * 0` → `0`, `x / x` → `1`. Esas son
correctas matemáticamente pero **descartan** el operando `x`, y si `x` tuviera un
efecto secundario (una llamada a función, un `++`) ese efecto se perdería. El pase
evita ese riesgo por completo limitándose a casos donde el operando descartado es
siempre el literal `0`/`1`, que no tiene efectos.

---

## 2. La estructura: hijos primero, luego este nodo

Igual que el folder, simplifica de abajo hacia arriba:

```cpp
void AlgebraicSimplifier::visit(BinaryExpr* node) {
    AstWalker::visit(node);   // simplifica los hijos primero
    Expr* L = node->left;
    Expr* R = node->right;
    switch (node->op) {
        case BinaryOp::Add:
            if      (isZero(R)) replaceWith(detach(node->left));   // x + 0 → x
            else if (isZero(L)) replaceWith(detach(node->right));  // 0 + x → x
            break;
        case BinaryOp::Sub:
            if (isZero(R)) replaceWith(detach(node->left));        // x - 0 → x
            break;
        case BinaryOp::Mul:
            if      (isOne(R)) replaceWith(detach(node->left));    // x * 1 → x
            else if (isOne(L)) replaceWith(detach(node->right));   // 1 * x → x
            break;
        case BinaryOp::Div:
            if (isOne(R)) replaceWith(detach(node->left));         // x / 1 → x
            break;
        default: break;
    }
}
```

`isZero`/`isOne` son los helpers de `opt_util.h` (`2_constant_folding.md §2`), que
reconocen `0`/`0.0`/`false` y `1`/`1.0`/`true` respectivamente. Las asimetrías son
intencionales: `0 - x` **no** es `x` (es `-x`), por eso `Sub` solo mira el lado
derecho; y `1 / x` **no** es `x`, por eso `Div` también.

---

## 3. El detalle clave: `detach`

Aquí aparece una sutileza de gestión de memoria. El mecanismo de reemplazo de
`AstWalker` (`1_optimizer.md §4`) **borra el nodo viejo** cuando se hace
`replaceWith`. Pero en este pase, el reemplazo es **uno de los hijos del propio
nodo viejo** — no un nodo nuevo.

Si simplemente hiciéramos `replaceWith(node->left)`, pasaría un desastre: `walk`
borraría el `BinaryExpr`, cuyo destructor a su vez hace `delete left; delete
right;`... incluyendo el nodo que acabábamos de querer conservar. El reemplazo
quedaría apuntando a memoria liberada.

La solución es `detach`: **desengancha** el hijo del nodo poniéndolo a `nullptr`
antes de que el destructor pueda alcanzarlo.

```cpp
static Expr* detach(Expr*& slot) {
    Expr* e = slot;
    slot = nullptr;   // el destructor del BinaryExpr ya no lo verá
    return e;
}
```

Así, cuando `walk` borre el `BinaryExpr`, su destructor hará `delete nullptr` (que
es seguro y no hace nada) sobre el slot vaciado, y `delete` sobre el literal `0`/`1`
sobrante — que sí queremos liberar. El operando rescatado sobrevive intacto como el
nuevo nodo del árbol.

---

## En contexto

El `AlgebraicSimplifier` corre **después** de la propagación de constantes
(`3_constant_propagation.md`), porque la propagación puede ser la que deje un
literal neutro a la vista: `int k = 0; ... y = x + k` se vuelve `y = x + 0` tras
propagar, y entonces esta simplificación lo reduce a `y = x`. Junto con el folding,
forma el grueso de las optimizaciones de expresión; el `DeadCodeEliminator`
(`5_dead_code.md`) cierra la cadena trabajando ya no sobre expresiones sino sobre
sentencias.
