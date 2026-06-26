# Reglas Semánticas del Compilador

Reglas verificables sobre la gramática definida en `grammar.md`.  
Notación: **error** = el checker lanza `SemanticError`. **warn** = advertencia (opcional).

---

## 1. Tipos y Compatibilidad

### 1.1 Tipos válidos
- Los tipos base permitidos son: `int`, `float`, `bool`, `char`, `string`, `void`.
- Un tipo de usuario (identificador) solo es válido si un `struct` con ese nombre fue declarado antes o en la primera pasada.
- `void` **solo** es válido como tipo de retorno de función. Usarlo como tipo de variable es **error**.
- `auto` requiere inicializador para inferir el tipo; sin inicializador es **error**.

### 1.2 Compatibilidad implícita (coerción permitida)
Estas conversiones se aceptan de forma implícita:

| De       | A                   |
|----------|---------------------|
| `bool`   | `int`, `float`      |
| `char`   | `int`, `float`      |
| `int`    | `float`             |
| numérico | `bool` (en condiciones) |

No se permite conversión implícita entre `string`, structs, punteros y tipos numéricos.

---

## 2. Declaraciones Globales

### 2.1 Structs (`StructDecl`)
- El nombre del struct no puede repetirse globalmente → **error**.
- Ningún miembro puede tener tipo `void` o `auto` → **error**.
- Dos miembros con el mismo nombre → **error**.
- El tipo de un miembro puede ser otro struct solo si ese struct ya fue declarado (primera pasada).

### 2.2 Funciones (`FuncDecl`)
- El nombre de función no puede repetirse en el scope global → **error** (no hay sobrecarga).
- El tipo de retorno debe ser un tipo válido.
- Los nombres de los parámetros deben ser únicos dentro de la lista → **error**.
- Parámetro con tipo `void` → **error**.
- Si el retorno es `void`: cualquier `return expr` dentro es **error**.
- Si el retorno no es `void`: debe existir al menos un `return expr` en el cuerpo → **error** si no hay ninguno.
- El tipo del valor retornado debe ser compatible con el tipo de retorno declarado → **error** si no.

### 2.3 Funciones template (`TemplateFuncDecl`)
- El nombre del parámetro de tipo (ej. `T`) entra al scope de la función como tipo válido.
- Se aplican las mismas reglas que `FuncDecl`.
- Usar `T` como tipo en cualquier expresión o declaración dentro es válido.

---

## 3. Statements

### 3.1 Declaración de variable local (`VarDeclStmt`)
- No puede redeclararse el mismo nombre en el **mismo** scope (bloque actual) → **error**.  
  Shadowing de scopes exteriores **se permite** (igual que C++).
- Tipo `void` → **error**.
- Si tiene inicializador, el tipo debe ser compatible → **error**.
- **Array estático**: cada dimensión debe ser de tipo entero (`int`) → **error**. La variable decae a puntero (un nivel por dimensión: `int arr[3]` → `int*`), por eso `arr[i]` queda tipado.
- Si tiene `init_list`, cada elemento debe ser compatible con el tipo base del array → **error**.
- Si usa `auto`, el inicializador infiere el tipo; la variable toma ese tipo de ahí en adelante.

### 3.2 `if`
- La condición debe ser `bool` o un tipo numérico (convertible a `bool`) → **error** si es `string`, struct, o puntero sin deref.

### 3.3 `while`
- Ídem condición que `if`.

### 3.4 `for` clásico
- El init puede ser `VarDeclStmt` o expresión; su scope es el propio bloque del `for`.
- La condición, si existe, debe ser `bool` o numérico → **error**.
- El update puede ser cualquier tipo de expresión.

### 3.5 `return`
- Debe estar dentro de una función o lambda → **error** si está en el scope global.
- Si la función retorna `void`: `return expr` es **error**; `return;` es válido.
- Si la función retorna no-`void`: `return;` es **error**; el tipo de la expresión debe ser compatible → **error**.

### 3.6 `break` / `continue`
- Solo válidos dentro de `while` o `for` → **error** en cualquier otro contexto.

### 3.7 `delete`
- La expresión debe ser de tipo puntero → **error** si no lo es.
- `delete[]` debe usarse para punteros obtenidos con `new Type[n]`.

---

## 4. Expresiones

### 4.1 Literales
| Literal       | Tipo inferido |
|---------------|---------------|
| `IntLit`      | `int`         |
| `FloatLit`    | `float`       |
| `true/false`  | `bool`        |
| `CharLit`     | `char`        |
| `StringLit`   | `string`      |

### 4.2 Identificador (`IdExpr`)
- Debe estar declarado en algún scope visible → **error** si no existe.
- El tipo es el declarado en su `VarDeclStmt` / `Param`.

### 4.3 Operadores binarios (`BinaryExpr`)

**Aritméticos** (`+`, `-`, `*`, `/`, `%`):
- Ambos operandos deben ser numéricos → **error**.
- `%` solo para `int` → **error** con `float`.
- Resultado: se aplica promoción numérica (el tipo "mayor" de los dos).

**Comparación** (`==`, `!=`, `<`, `>`, `<=`, `>=`):
- Operandos deben ser del mismo tipo o compatibles (numéricos, `char`, `bool`) → **error** para `string` vs numérico, struct, etc.
- Resultado: `bool`.

**Lógicos** (`&&`, `||`):
- Operandos deben ser `bool` o convertibles (`bool`, `int`, `float`) → **error** para `string`, struct.
- Resultado: `bool`.

### 4.4 Operadores unarios (`UnaryExpr`)
- `-` (negación): operando numérico → resultado mismo tipo.
- `!` (not lógico): operando `bool` o numérico → resultado `bool`.
- `*` (deref): operando debe ser puntero (`T*`) → resultado `T` → **error** si no es puntero.
- `&` (address-of): operando debe ser lvalue → resultado `T*`.
- `++` / `--` prefijos: operando debe ser lvalue de tipo numérico o puntero → resultado mismo tipo.

### 4.5 Asignación (`AssignExpr`)
- El lado izquierdo debe ser un **lvalue**: `IdExpr`, `IndexExpr`, `MemberExpr`, o `UnaryExpr` con `Deref` → **error** para literales y otros.
- El tipo del lado derecho debe ser compatible con el izquierdo → **error**.
- `+=`, `-=`, `*=`, `/=`: lvalue debe ser numérico → **error**.
- Resultado de la expresión: tipo del lvalue.

### 4.6 Llamadas a función (`CallExpr`)
- El callee debe ser una función declarada, un built-in, o un valor de tipo función (variable con una lambda, o lambda inline) → **error** si no existe o no es invocable.
- El número de argumentos debe coincidir con el número de parámetros → **error**.
- El tipo de cada argumento debe ser compatible con el tipo del parámetro correspondiente → **error**.
- Pasar por referencia (`&`): se requiere lvalue como argumento → **error** con literales.
- Resultado: tipo de retorno de la función.

### 4.7 Built-ins `print` / `println`
- Aceptan cualquier número de argumentos ≥ 1.
- Cada argumento debe ser de tipo básico: `int`, `float`, `bool`, `char`, `string` → **error** para structs o punteros.
- Resultado: `void`.

### 4.8 Acceso a miembro (`MemberExpr`)
- Con `.`: el base debe ser de tipo struct (no puntero) → **error**.
- Con `->`: el base debe ser puntero a struct (`S*`) → **error**.
- El nombre del miembro debe existir en el struct → **error**.
- Resultado: tipo del miembro accedido.

### 4.9 Indexado (`IndexExpr`)
- El base debe ser de tipo array o puntero → **error** para otros tipos.
- El índice debe ser de tipo `int` → **error**.
- Resultado: tipo del elemento (el tipo base sin el modificador de puntero/array).

### 4.10 Postfix `++` / `--`
- Base debe ser lvalue de tipo numérico o puntero → **error**.
- Resultado: tipo del base (valor antes de la operación).

### 4.11 `new`
- `new Type[n]`: tipo no puede ser `void` → **error**; `n` debe ser entero → **error**. Resultado: `Type*`.
- `new Type`: el tipo debe ser un struct declarado → **error**. Reserva un objeto con sus campos en cero. Resultado: `Type*`.

### 4.12 Lambda (`LambdaExpr`)
- El tipo de la lambda es su firma (`params -> retorno`); puede guardarse en `auto` y llamarse (ver 4.6).
- Solo se permiten lambdas **sin capturas** (`[]`). Una lista de captura no vacía → **error**.
- Sin capturas, el cuerpo solo puede usar sus propios parámetros y funciones; referenciar una variable local del scope exterior → **error**.
- Las reglas del cuerpo son iguales a las de una función normal.
- Si hay tipo de retorno explícito (`-> Type`), se verifican los `return` igual que en una función.
- Si no hay tipo de retorno explícito, se infiere del primer `return` encontrado.

---

## 5. Reglas de Scope

- Scopes: global → función/lambda → bloque → bloques anidados.
- Una variable no puede usarse antes de su declaración dentro del mismo scope → **error**.
- Funciones y structs son visibles en todo el programa (recolectados en primera pasada).
- Variables declaradas en el `for`-init existen solo dentro del `for`.
- Los parámetros de una función existen solo dentro de su cuerpo.

---

## 6. Primera Pasada (recolección)

Antes del chequeo completo, se recorren todas las declaraciones de nivel superior para registrar:
- Todos los structs (nombre + miembros).
- Todas las funciones (nombre + firma completa).

Esto permite que funciones se llamen entre sí sin importar el orden de declaración.

---

## 7. Errores vs. Advertencias

El checker lanza `SemanticError` (similar a `ParseError`) con mensaje descriptivo.  
Se detiene en el primer error encontrado (modo fail-fast, igual que el parser).

Posibles **advertencias** (no implementadas como error, pero detectables):
- Shadowing de variable de scope externo con el mismo nombre.
- Variable declarada pero nunca usada.
- Función con retorno no-void sin `return` en alguna rama (análisis de flujo completo es opcional).
