# Análisis Semántico (TypeChecker)

Tras el **Parser**, el AST ya es sintácticamente válido, pero todavía puede tener errores de *significado*: usar una variable no declarada, sumar un `string` con un `int`, llamar una función con argumentos de más, etc. El **análisis semántico** recorre el AST (otra vez con el patrón Visitor) y verifica todas esas reglas.

La fase la implementa la clase `TypeChecker` (`semantic/type_checker.{h,cpp}`), apoyada en dos piezas auxiliares: `SemType` (representación de tipos resueltos) y `SymbolTable` (scopes). Las reglas concretas que valida están en [semantic_rules.md].

---

## 1. `SemType` — el tipo resuelto

Mientras el AST usa `TypeNode` (la forma *sintáctica* del tipo, tal como se escribió), el semántico trabaja con `SemType`, una versión limpia y comparable:

```cpp
struct SemType {
    std::string         base;   // "int", "float", "MiStruct", ...
    std::vector<PtrMod> mods;   // modificadores en orden: *, &
};
```

Sus operaciones clave:

* **`accepts(other)`**: ¿se puede asignar/pasar `other` donde se espera `this`? Acepta el mismo tipo exacto o una **promoción numérica** implícita (`bool` → `char` → `int` → `float`). Los punteros solo aceptan punteros del mismo tipo.
* **`promote(a, b)`**: dado dos tipos numéricos, devuelve el "mayor" (el de mayor rango). Es lo que decide que `int + float` sea `float`.
* **`fromTypeNode(node)`**: convierte el `TypeNode` del AST a `SemType`.
* Consultas: `isNumeric()`, `isIntegral()`, `isVoid()`, `hasPointer()`, `deref()`, etc.

El orden de promoción se define en un único lugar (`numericRank`): `bool=0, char=1, int=2, float=3`.

---

## 2. `SymbolTable<T>` — manejo de scopes

Tabla de símbolos genérica (`semantic/symbol_table.h`), una pila de mapas `nombre → T`:

```cpp
std::vector<std::unordered_map<std::string, T>> scopes;
```

* **`enterScope()` / `exitScope()`**: entran y salen de un bloque (no se puede salir del global).
* **`declare(name, info)`**: declara en el scope **actual**; retorna `false` si el nombre ya existe en ese scope (permite *shadowing* de scopes externos, igual que C++).
* **`lookup(name)`**: busca desde el scope más interno hacia afuera; retorna `nullptr` si no existe.

El `TypeChecker` la instancia como `SymbolTable<VarInfo>` para las variables. Funciones y structs viven en `unordered_map` aparte porque son globales.

---

## 3. Las dos pasadas

### Primera pasada (`firstPass`)

Recorre solo las declaraciones de nivel superior y registra sus **firmas**, sin entrar a los cuerpos. Así una función puede llamar a otra declarada más abajo, o un struct referenciar otro.

Se hace en dos sub-pasadas para resolver referencias cruzadas:
1. Registrar los **nombres** de todos los structs (vacíos).
2. Llenar miembros de structs, firmas de funciones (`FuncInfo`) y variables globales.

### Segunda pasada (los `visit`)

Recorre el AST completo verificando cada nodo. Cada `visit` de expresión deja el tipo resultante en el miembro `expr_type_`, que el nodo padre consulta vía el helper `visitExpr(e)`:

```cpp
SemType TypeChecker::visitExpr(Expr* e) {
    e->accept(this);
    return expr_type_;   // tipo que dejó el visit
}
```

> Es el mismo mecanismo de "devolver el tipo por un miembro" que usa el CodeGenerator con `cur_type_`, porque `visit(...)` retorna `void`.

---

## 4. Información que mantiene

Durante el recorrido el checker guarda contexto en structs auxiliares y banderas:

| Campo | Para qué |
|---|---|
| `vars_` (`SymbolTable<VarInfo>`) | variables visibles |
| `funcs_` (`map → FuncInfo`) | firmas de funciones y built-ins |
| `structs_` (`map → StructInfo`) | miembros de cada struct |
| `ret_type_` | tipo de retorno de la función actual (para validar `return`) |
| `in_loop_` | si estamos dentro de un loop (para `break`/`continue`) |
| `in_template_`, `template_param_` | si estamos en una función template y el nombre del `typename` |
| `expr_type_` | tipo de la última expresión evaluada |

Los built-ins `print` y `println` se registran al construir el checker (`registerBuiltins`) como funciones variádicas que retornan `void`.

---

## 5. Manejo de errores

Igual que el parser, el checker es **fail-fast**: lanza `SemanticError` (con línea y columna) en el primer problema.

```cpp
struct SemanticError : std::runtime_error {
    int line, col;
};
```

Helpers de chequeo frecuentes:
* **`resolveType(node)`**: convierte un `TypeNode` a `SemType` validando que el tipo base exista (tipo primitivo, struct declarado, o el `T` de un template).
* **`isLvalue(e)`**: si una expresión puede ir a la izquierda de una asignación (`IdExpr`, `IndexExpr`, `MemberExpr`, o `*ptr`).
* **`isArithmetic(t)` / `isCondition(t)`**: tipos válidos en operaciones numéricas / en condiciones de `if`/`while`.
* **`bodyHasReturn(s)`**: análisis simple de flujo para exigir `return` en funciones no-`void`.
