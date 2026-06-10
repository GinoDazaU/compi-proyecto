# Árbol de Sintaxis Abstracta (AST)

El AST (Abstract Syntax Tree o Árbol de Sintaxis Abstracta) es la estructura intermedia más importante del compilador. Después de que el Lexer agrupa los caracteres en Tokens, el Parser toma esos tokens y construye un árbol que representa la estructura gramatical del programa.

El archivo ast.h define las clases para cada tipo de nodo en este árbol. Vamos a desglosarlo por partes:

---

### 1. Las Clases Base y Jerarquía Principal

Para poder organizar el código, todos los nodos del árbol se dividen en tres grandes familias heredadas de clases base virtuales:

1. **TopDecl (Declaraciones Globales)**: Representa cualquier elemento que se declara al nivel más alto del archivo (fuera de las funciones), como:
   * Funciones (`FuncDecl`)
   * Estructuras (`StructDecl`)
   * Variables globales (`GlobalVarDecl`)
   * Plantillas (`TemplateFuncDecl`)

2. **Stmt (Sentencias/Instrucciones)**: Representa acciones que ejecutan lógica pero no producen un valor de retorno directo en sí mismas:
   * Declaración de variables locales (`VarDeclStmt`)
   * Estructuras de control (`IfStmt`, `WhileStmt`, `ForStmt`)
   * Retornos (`ReturnStmt`), `break`, `continue`, `delete`
   * Un bloque de sentencias agrupadas entre llaves `{}` (`Block`)

3. **Expr (Expresiones)**: Cualquier código que al ejecutarse se evalúa y produce un valor y un tipo (ej. `x + 42` devuelve un entero):
   * Literales (`IntLitExpr`, `FloatLitExpr`, `StringLitExpr`...)
   * Variables (`IdExpr`)
   * Operaciones (`BinaryExpr`, `UnaryExpr`, `AssignExpr`)
   * Memoria dinámica (`NewObjectExpr`, `NewArrayExpr`)
   * Funciones anónimas (`LambdaExpr`)

El nodo raíz de todo el archivo de código es la clase **Program**, que simplemente guarda una lista de declaraciones de nivel superior:
```cpp
class Program {
public:
    std::vector<TopDecl*> decls; // Lista de funciones, structs, variables globales...
};
```

---

### 2. Representación de Tipos (`TypeNode`)

C++ permite tipos muy complejos (ej. `const int*&` o `vector<int>`). Para modelar esto, `ast.h` define `TypeNode`:

```cpp
struct TypeNode {
    bool                is_const = false;  // ¿Tiene const?
    bool                is_auto  = false;  // ¿Usa inferencia de tipo auto?
    std::string         base;              // Tipo base: "int", "float", "MiStruct"
    TypeNode*           template_arg = nullptr; // Para tipos genéricos como vector<T>
    std::vector<PtrMod> mods;              // Modificadores (* o &)
};
```
* `PtrMod` es un enum con dos opciones: `Pointer` (`*`) o `Reference` (`&`).
* Si el tipo es `const int*&`, el vector `mods` guardará `[PtrMod::Pointer, PtrMod::Reference]`.

---

### 3. Anatomía de las Expresiones (`Expr`)

Las expresiones se evalúan para dar un valor. Veamos algunos ejemplos de cómo se modelan:

#### A. Operaciones Binarias (`BinaryExpr`)
Representa operaciones con un operador al centro y dos lados, como `5 + x`:
```cpp
class BinaryExpr : public Expr {
public:
    Expr*    left;   // Puntero al sub-árbol del lado izquierdo (ej: IntLitExpr(5))
    BinaryOp op;     // Operador (Add, Sub, Mul, Eq...)
    Expr*    right;  // Puntero al sub-árbol del lado derecho (ej: IdExpr("x"))
};
```

#### B. Llamadas a funciones (`CallExpr`)
Representa algo como `calcular(10, y)`:
```cpp
class CallExpr : public Expr {
public:
    Expr*              callee; // La función que llamamos (ej: IdExpr("calcular"))
    std::vector<Expr*> args;   // Los argumentos pasados: [IntLitExpr(10), IdExpr("y")]
};
```

---

### 4. Anatomía de las Sentencias (`Stmt`)

Las sentencias guían el flujo de ejecución:

#### A. La sentencia Condicional (`IfStmt`)
Representa `if (condicion) { ... } else { ... }`:
```cpp
class IfStmt : public Stmt {
public:
    Expr* condition;   // La expresión que evalúa a bool
    Stmt* then_branch; // Bloque que se ejecuta si es true (siempre es un Block*)
    Stmt* else_branch; // Bloque o IfStmt (para 'else if') si es false (puede ser nullptr)
};
```

#### B. Bucles `For` clásicos y de Rango (`ForStmt` / `ForRangeStmt`)
El AST soporta tanto el bucle clásico:
* `for (int i = 0; i < 10; i++)` -> mapeado por `ForStmt`
Como el bucle de rango moderno de C++:
* `for (const auto x : mi_lista)` -> mapeado por `ForRangeStmt`

---

### 5. Gestión de Memoria en el AST

En este diseño se están utilizando punteros crudos (raw pointers) (ej. `Expr*`, `Stmt*`). Por lo tanto, para evitar fugas de memoria (memory leaks), cada nodo tiene un destructor personalizado encargado de hacer `delete` recursivo de sus hijos.

Por ejemplo, el destructor de `FuncDecl` (declaración de función):
```cpp
~FuncDecl() override {
    delete return_type;
    for (auto& p : params) { delete p.type; delete p.default_val; }
    delete body;
}
```
Cuando borras el nodo de la función, automáticamente libera el tipo de retorno, todos los parámetros y el bloque de código del cuerpo.
