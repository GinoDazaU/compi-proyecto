# Constant Folding (ConstantFolder)

El *constant folding* (plegado de constantes) es la optimización más clásica:
**calcular en tiempo de compilación toda operación cuyos operandos ya son
literales**. Si el código dice `2 + 3`, no tiene sentido emitir un `addq` que el
procesador ejecute en cada corrida; el compilador puede resolver `5` una sola vez
y dejar ese literal en el AST.

Lo implementa la clase `ConstantFolder` (en `constant_folder.h` / `.cpp`), que
hereda de `AstWalker` y solo sobreescribe los dos nodos que puede plegar:

```cpp
class ConstantFolder : public AstWalker {
public:
    void visit(BinaryExpr* node) override;
    void visit(UnaryExpr* node) override;
};
```

Todo lo demás —el recorrido del árbol y el mecanismo de reemplazo— lo hereda de
`AstWalker` (ver `1_optimizer.md`).

---

## 1. El patrón: hijos primero, luego este nodo

Cada `visit` sigue siempre el mismo orden, y es importante entender por qué:

```cpp
void ConstantFolder::visit(BinaryExpr* node) {
    AstWalker::visit(node);   // 1. pliega left/right primero (recursión)
    // 2. ahora intenta plegar ESTE nodo
    ...
}
```

Primero se llama a `AstWalker::visit(node)`, que recorre `left` y `right` con
`walk` y los pliega si se puede. **Solo después** se intenta plegar el nodo
actual. Este orden *bottom-up* es lo que permite plegar expresiones anidadas en
una sola pasada: en `(1 + 2) * 3`, primero `1 + 2` se vuelve `3`, y recién
entonces `3 * 3` se puede plegar a `9`. Si lo hiciéramos al revés, al mirar la
multiplicación todavía veríamos un `BinaryExpr` a la izquierda, no un literal.

---

## 2. Inspeccionar literales: `opt_util.h`

Para decidir si un operando es literal y obtener su valor, los pases comparten un
puñado de helpers en `opt_util.h` (namespace `optutil`). Son `inline` y se basan
en `dynamic_cast` sobre los nodos del AST:

```cpp
// ¿Es un literal numérico (int/float/bool)? Devuelve su valor como double.
inline bool asDouble(Expr* e, double& out);

// ¿Es un literal entero (int/bool)? Devuelve su valor como long long.
inline bool asLong(Expr* e, long long& out);

inline bool isFloatLit(Expr* e);   // ¿es FloatLitExpr?
inline bool isZero(Expr* e);       // ¿literal con valor 0 / 0.0 / false?
inline bool isOne(Expr* e);        // ¿literal con valor 1 / 1.0 / true?
```

`asDouble` y `asLong` cumplen doble función: el booleano dice *si* el nodo es
literal, y el parámetro de salida entrega *el valor*. Que `bool` cuente como
numérico (`true` → `1`, `false` → `0`) refleja las reglas de coerción del lenguaje
(ver `semantic_rules.md §1.2`). `isZero`/`isOne` los usa sobre todo el
`AlgebraicSimplifier` (`4_algebraic_simplification.md`).

---

## 3. Plegar binarias: int vs. float

Una vez plegados los hijos, el folder comprueba que **ambos** sean literales
numéricos; si alguno no lo es, se rinde y no hace nada:

```cpp
double ld, rd;
if (!asDouble(node->left, ld) || !asDouble(node->right, rd)) return;

long long li, ri;
bool bothInt = asLong(node->left, li) && asLong(node->right, ri);
```

`bothInt` distingue los dos caminos, y es clave para **no cambiar el tipo** del
resultado:

- Si los dos operandos son enteros, el cálculo se hace en `long long` y el
  reemplazo es un `IntLitExpr`. Así `7 / 2` da `3` (división entera), igual que en
  runtime.
- Si alguno es `float`, el cálculo se hace en `double` y el reemplazo es un
  `FloatLitExpr`.

```cpp
case BinaryOp::Add: case BinaryOp::Sub: case BinaryOp::Mul:
case BinaryOp::Div: case BinaryOp::Mod:
    if (bothInt) {
        if ((node->op == BinaryOp::Div || node->op == BinaryOp::Mod) && ri == 0)
            return;                                  // ¡no plegar /0 ni %0!
        long long r = /* li (op) ri */;
        replaceWith(new IntLitExpr(r));
    } else {
        double r = /* ld (op) rd */;
        if (node->op == BinaryOp::Div && rd == 0) return;  // tampoco /0.0
        if (node->op == BinaryOp::Mod) return;             // % no aplica a float
        replaceWith(new FloatLitExpr(r));
    }
    break;
```

Dos detalles de **seguridad** que valen la regla de oro "ante la duda, no
optimizar":

- **División/módulo por cero no se pliega.** Si el divisor es `0`, el folder
  retorna sin tocar el nodo. Plegarlo significaría decidir en compilación un
  comportamiento que es del runtime; mejor dejar que la operación ocurra (o falle)
  como lo haría sin optimizar.
- **`%` solo entre enteros.** Coincide con la regla semántica de que el módulo
  exige operandos `int`.

---

## 4. Plegar comparaciones y lógicos

Las comparaciones y los operadores lógicos siempre producen `bool`, así que su
reemplazo es un `BoolLitExpr`. Como todos los operandos ya se normalizaron a
`double`, basta comparar:

```cpp
case BinaryOp::Eq:  replaceWith(new BoolLitExpr(ld == rd)); break;
case BinaryOp::Lt:  replaceWith(new BoolLitExpr(ld <  rd)); break;
// ... Neq, Gt, Leq, Geq igual ...
case BinaryOp::And: replaceWith(new BoolLitExpr(ld != 0 && rd != 0)); break;
case BinaryOp::Or:  replaceWith(new BoolLitExpr(ld != 0 || rd != 0)); break;
```

Esto es lo que convierte `if (2 > 1)` en `if (true)` — un literal que más tarde el
`DeadCodeEliminator` aprovecha para borrar la rama muerta.

---

## 5. Plegar unarios

`visit(UnaryExpr)` cubre los dos operadores unarios que tienen sentido sobre
literales: la negación aritmética `-` y la negación lógica `!`. (`*`, `&`, `++`,
`--` operan sobre lvalues, no sobre literales, así que no se pliegan.)

```cpp
void ConstantFolder::visit(UnaryExpr* node) {
    AstWalker::visit(node);  // pliega el operando primero

    if (node->op == UnaryOp::Neg) {
        long long li; double ld;
        if (asLong(node->expr, li))             replaceWith(new IntLitExpr(-li));
        else if (isFloatLit(node->expr) && asDouble(node->expr, ld))
                                                replaceWith(new FloatLitExpr(-ld));
    } else if (node->op == UnaryOp::Not) {
        double d;
        if (asDouble(node->expr, d)) replaceWith(new BoolLitExpr(d == 0));
    }
}
```

`Neg` preserva el tipo igual que las binarias: `-3` queda `int`, `-3.0` queda
`float`. `Not` siempre da `bool` (`!0` → `true`, `!5` → `false`).

---

## En contexto

El `ConstantFolder` es el pase más fundamental y por eso corre **dos veces** en el
pass manager (`1_optimizer.md §1`): una al inicio para plegar lo que ya es
literal, y otra después del `ConstantPropagator`
(`3_constant_propagation.md`), que expone literales nuevos al reemplazar variables
constantes. Sus resultados alimentan al `AlgebraicSimplifier` y al
`DeadCodeEliminator`, que dependen de que las condiciones y operandos ya estén
plegados.
