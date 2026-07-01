# Análisis Semántico (TypeChecker)

Tras el parser, el AST ya es **sintácticamente** válido, pero todavía puede tener
errores de *significado*: usar una variable no declarada, sumar un `string` con un
`int`, llamar una función con argumentos de más, retornar el tipo equivocado. El
**análisis semántico** recorre el AST (otra vez con el patrón Visitor) y verifica
todas esas reglas.

Lo implementa la clase `TypeChecker`, en `type_checker.h` (las estructuras de
información) y `type_checker.cpp` (toda la lógica). Se apoya en dos piezas con
documento propio: `SemType` (tipos resueltos) y `SymbolTable` (scopes). La lista
completa de reglas que valida está en `semantic_rules.md`; aquí vemos **cómo** está
escrito el checker.

---

### 1. Por qué dos pasadas

Un programa puede usar una función **antes** de declararla:

```cpp
int main() { return cuadrado(3); }   // se usa aquí...
int cuadrado(int x) { return x*x; }  // ...y se declara después
```

Si recorriéramos el AST de un tirón, al llegar a `cuadrado(3)` no sabríamos qué es
`cuadrado`. Por eso el punto de entrada hace **dos pasadas**:

```cpp
void TypeChecker::check(Program* program) {
    firstPass(program);       // recolecta firmas globales (structs, funciones)
    program->accept(this);    // recorre y verifica todo a fondo
}
```

---

### 2. La primera pasada (`firstPass`)

Recorre **solo** las declaraciones de nivel superior, sin entrar a los cuerpos, y
en dos sub-pasadas para resolver referencias cruzadas:

```cpp
// Sub-pasada 1: registrar los NOMBRES de los structs (vacíos)
for (auto* decl : program->decls)
    if (auto* s = dynamic_cast<StructDecl*>(decl))
        structs_[s->name] = {};

// Sub-pasada 2: llenar miembros de structs y firmas de funciones
for (auto* decl : program->decls) {
    if (auto* s = dynamic_cast<StructDecl*>(decl)) { /* ... miembros ... */ }
    else if (auto* f = dynamic_cast<FuncDecl*>(decl)) { /* ... FuncInfo ... */ }
}
```

Primero solo los **nombres** de los structs, para que un struct pueda mencionar a
otro definido más abajo; recién en la segunda vuelta se llenan miembros y firmas.
Al terminar, `funcs_` y `structs_` conocen todo el "vocabulario global", sin
importar el orden de declaración.

---

### 3. La segunda pasada y `expr_type_`

Aquí aparece un detalle técnico del patrón Visitor: `visit(...)` devuelve `void`,
así que un `visit` de expresión **no puede retornar** el tipo que calculó. La
solución es dejarlo en un miembro, `expr_type_`, y leerlo con un helper:

```cpp
SemType TypeChecker::visitExpr(Expr* e) {
    e->accept(this);
    return expr_type_;   // el tipo que el visit acaba de dejar
}
```

Así un nodo padre escribe `SemType lt = visitExpr(node->left);` y obtiene el tipo
del hijo. La regla del checker es: **todo `visit` de expresión termina asignando
`expr_type_`**. (El codegen usa exactamente el mismo truco con su miembro
`cur_type_`.)

El caso más simple lo deja ver claro, `IdExpr`:

```cpp
void TypeChecker::visit(IdExpr* node) {
    VarInfo* v = vars_.lookup(node->name);
    if (v) { expr_type_ = v->type; return; }                 // es una variable
    if (funcs_.count(node->name)) { expr_type_ = SemType{"void"}; return; }
    semError("use of undeclared variable '" + node->name + "'", node->line, node->col);
}
```

Busca el nombre en la tabla de símbolos; si no es variable ni función, error.

---

### 4. Un `visit` con verificación: `BinaryExpr`

Este nodo muestra el patrón completo: evaluar los hijos, validar, y dejar el tipo
del resultado.

```cpp
void TypeChecker::visit(BinaryExpr* node) {
    SemType lt = visitExpr(node->left);
    SemType rt = visitExpr(node->right);
    switch (node->op) {
        case BinaryOp::Add: case BinaryOp::Sub:
        case BinaryOp::Mul: case BinaryOp::Div:
            if (!isArithmetic(lt) || !isArithmetic(rt))
                semError("arithmetic operands must be numeric", node->line, node->col);
            expr_type_ = SemType::promote(lt, rt);     // char + int → int
            break;
        case BinaryOp::Mod:
            if (!lt.isIntegral() || !rt.isIntegral())
                semError("'%' requires int operands", node->line, node->col);
            expr_type_ = SemType{"int"};
            break;
        case BinaryOp::Eq: case BinaryOp::Neq: /* < > <= >= */
            if (!isArithmetic(lt) || !isArithmetic(rt))
                semError("incompatible operands in comparison", node->line, node->col);
            expr_type_ = SemType{"bool"};              // comparar da bool
            break;
        case BinaryOp::And: case BinaryOp::Or:
            if (!isCondition(lt) || !isCondition(rt))
                semError("logical operands must be bool or numeric", node->line, node->col);
            expr_type_ = SemType{"bool"};
            break;
    }
}
```

Nota cómo cada operador decide su propio tipo de salida: la aritmética **promueve**
(`SemType::promote`), `%` exige enteros y da `int`, y las comparaciones y lógicos
siempre dan `bool`.

---

### 5. Helpers de validación

Las decisiones que se repiten en muchos `visit` están en funciones cortas. Por
ejemplo, qué tipos sirven para aritmética y para condiciones:

```cpp
bool TypeChecker::isArithmetic(const SemType& t) const {
    return !t.hasPointer() &&
           (t.base == "int" || t.base == "float" ||
            t.base == "bool" || t.base == "char");
}
```

Y cuáles expresiones pueden ir a la izquierda de un `=` (un *lvalue*):

```cpp
bool TypeChecker::isLvalue(Expr* e) const {
    if (dynamic_cast<IdExpr*>(e))     return true;   // x
    if (dynamic_cast<IndexExpr*>(e))  return true;   // arr[i]
    if (dynamic_cast<MemberExpr*>(e)) return true;   // s.x
    if (auto* u = dynamic_cast<UnaryExpr*>(e))
        return u->op == UnaryOp::Deref;              // *p
    return false;
}
```

Otros del mismo estilo: `resolveType` (convierte un `TypeNode` a `SemType`
validando que el tipo exista), `isCondition` (tipos válidos en `if`/`while`), y
`bodyHasReturn`, un análisis de flujo *simple* que exige `return` en funciones
no-`void` (un `if/else` solo cuenta si **ambas** ramas retornan; `while`/`for` no
garantizan ejecución, así que no cuentan).

---

### 6. El estado que se arrastra

Algunos chequeos dependen de *dónde* estamos. El checker guarda ese contexto en
banderas que se **salvan y restauran** al entrar/salir de cada función. Mira
`visit(FuncDecl)`:

```cpp
void TypeChecker::visit(FuncDecl* node) {
    ret_type_      = resolveType(node->return_type, node->line, node->col);
    bool prev_loop = in_loop_;     // guardar
    in_loop_       = false;        // dentro de la función todavía no hay loop
    vars_.enterScope();
    // ... declarar parámetros, verificar el cuerpo ...
    vars_.exitScope();
    in_loop_ = prev_loop;          // restaurar
}
```

- `ret_type_` permite que `visit(ReturnStmt)` compruebe que lo retornado sea
  compatible con la firma.
- `in_loop_` permite que `break`/`continue` sepan si están dentro de un bucle.

Los built-ins `print` y `println` se registran en el constructor
(`registerBuiltins`) como funciones **variádicas** que retornan `void`; por eso no
necesitan declararse y aceptan cualquier número de argumentos básicos.

---

### 7. Errores

Igual que el parser, el checker es **fail-fast**: lanza `SemanticError` (con línea
y columna) en el primer problema y se detiene.

```cpp
struct SemanticError : std::runtime_error {
    int line, col;
};
```

`main.cpp` la captura y la imprime, o la serializa a JSON para el frontend.

---

### En contexto

El checker recibe el AST del parser, lo recorre con `SemType` y `SymbolTable`, y lo
devuelve **igual** — verificado, pero sin anotar. El generador de código vuelve a
calcular los tipos por su cuenta usando la misma lógica de `SemType`.
