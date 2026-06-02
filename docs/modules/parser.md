# Analizador Sintáctico (Parser)

El **Parser** (o Analizador Sintáctico) es la fase del compilador que recibe la secuencia plana de tokens generada por el **Lexer** y la transforma en una estructura jerárquica: el **Árbol de Sintaxis Abstracta (AST)**. 

Este parser está implementado utilizando la técnica de **Clasificación por Descenso Recursivo** (Recursive Descent Parsing), donde cada regla de la gramática formal (definida en [grammar.md]) se mapea directamente a una función miembro en C++.

---

### 1. Estructura de `parser.h`

La clase `Parser` mantiene un vector con todos los tokens leídos por adelantado (gracias a un enfoque de tokenización inicial o `tokenize()`) y realiza un seguimiento de la posición actual de lectura:

```cpp
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    Program* parse(); // Punto de entrada principal

private:
    std::vector<Token> tokens; // Stream de tokens del lexer
    size_t pos = 0;            // Posición actual en el vector
    
    // ...
};
```

---

### 2. Motor de Navegación del Parser

Para consumir tokens y avanzar de manera segura, el parser define varios métodos auxiliares de navegación:

1. **`cur()`**: Retorna una referencia al token actual en la posición `pos`.
2. **`peek(offset)`**: Permite inspeccionar tokens futuros sin consumirlos. Por defecto mira 1 posición adelante (`offset = 1`).
3. **`consume()`**: Retorna el token actual y avanza `pos` en 1.
4. **`check(type)`**: Devuelve `true` si el token actual coincide con el `TokenType` indicado.
5. **`match(type)`**: Si `check(type)` es verdadero, consume el token y retorna `true`. Si no, no avanza y retorna `false`.
6. **`expect(type)`**: Exige que el token actual sea del tipo especificado. Si lo es, lo consume; de lo contrario, lanza un error sintáctico.

---

### 3. Técnicas Especiales y Lookahead (Mirada hacia adelante)

Debido a que C++ es un lenguaje con ambigüedades sintácticas notables, el parser requiere a veces mirar múltiples tokens hacia adelante para decidir qué regla gramatical aplicar:

#### A. Detección de Tipos (`isTypeStart`)
Para distinguir entre una sentencia que declara una variable (`int x = 0;`) y una sentencia que es una expresión (`x = 0;`), el parser analiza si el token actual puede iniciar un tipo (keywords como `const`, `auto`, `int`, o un identificador de estructura seguido de un puntero `*`, referencia `&` o parámetros de template `<...>`):
```cpp
bool Parser::isTypeStart();
```

#### B. Detección de Range-based For (`isRangeFor`)
Al parsear un bucle `for`, el parser necesita distinguir si se trata de un `for` clásico (`for(int i = 0; i < 10; ++i)`) o un `for` de rango (`for(const auto& x : lista)`). El parser busca secuencialmente la presencia de un dos puntos (`:`) antes de encontrarse con un punto y coma (`;`):
```cpp
bool Parser::isRangeFor();
```

---

### 4. Manejo de Precedencia en Expresiones

El parser procesa las operaciones matemáticas, lógicas y relacionales de menor a mayor precedencia. Esto asegura que operaciones como `5 + 2 * 3` se agrupen correctamente como `5 + (2 * 3)` en el AST.

El flujo de llamadas recursivas para resolver la precedencia es el siguiente:

```
parseExpr()
 └── parseAssign()           (Menor precedencia: =, +=, -=, etc.)
      └── parseLogicOr()     (||)
           └── parseLogicAnd() (&&)
                └── parseEquality() (==, !=)
                     └── parseRelat() (<, >, <=, >=)
                          └── parseAdd() (+, -)
                               └── parseMul() (*, /, %)
                                    └── parseUnary() (-, !, ~, *, &, ++, --, static_cast, new)
                                         └── parsePostfix() ([], (), ., ->, ++/-- postfix)
                                              └── parsePrimary() (Mayor precedencia: literales, id, parentizados, lambdas)
```

#### Ejemplo: Operadores Asociativos por la Izquierda (`parseAdd`)
Para operadores como `+` o `-`, que se agrupan por la izquierda (`a - b - c` se evalúa como `(a - b) - c`), se utiliza un bucle `while`:
```cpp
Expr* Parser::parseAdd() {
    Expr* left = parseMul();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        BinaryOp op = match(TokenType::PLUS) ? BinaryOp::Add : (consume(), BinaryOp::Sub);
        left = new BinaryExpr(left, op, parseMul());
    }
    return left;
}
```

#### Ejemplo: Operadores Asociativos por la Derecha (`parseAssign`)
Para la asignación, que se agrupa por la derecha (`a = b = c` se evalúa como `a = (b = c)`), la función se llama de manera recursiva en su lado derecho:
```cpp
Expr* Parser::parseAssign() {
    Expr* left = parseLogicOr();
    // Si sigue un operador de asignación...
    // ...
    consume(); // Consumimos el operador
    Expr* right = parseAssign(); // Llamada recursiva
    return new AssignExpr(left, op, right);
}
```

---

### 5. Lambdas y Clausuras (`parseLambda`)

El subconjunto de C++ implementado incluye soporte para funciones anónimas (lambdas). El parser procesa su estructura completa:
* **Lista de capturas** (`[ & ]`, `[ = ]` o variables específicas `[x, &y]`).
* **Lista de parámetros** `(int a, float b)`.
* **Tipo de retorno opcional** `-> float`.
* **Cuerpo** entre llaves `{ ... }`.

```cpp
LambdaExpr* Parser::parseLambda() {
    expect(TokenType::LBRACKET);
    std::vector<CaptureItem> captures;
    // ... Lectura de capturas ...
    expect(TokenType::RBRACKET);
    expect(TokenType::LPAREN);
    auto params = parseParamList();
    expect(TokenType::RPAREN);
    // ...
    Block* body = parseBlock();
    return new LambdaExpr(std::move(captures), std::move(params), ret_type, body);
}
```

---

### 6. Sistema de Manejo de Errores Sintácticos

Si el código fuente no respeta las reglas gramaticales del lenguaje (por ejemplo, falta un punto y coma al final de una sentencia o un paréntesis sin cerrar), el parser detiene la ejecución inmediatamente y reporta un error estructurado a través de la excepción `ParseError`.

```cpp
struct ParseError : std::runtime_error {
    int line, col;
    ParseError(const std::string& msg, int line, int col)
        : std::runtime_error(msg), line(line), col(col) {}
};
```

Cuando se llama a `error("mensaje")` o `expect()`, se extraen los campos `line` y `col` del token inválido actual (`cur()`). Esto permite al compilador y a la interfaz web resaltar exactamente dónde ocurrió el fallo de compilación.

---

### 7. Integración con el AST y el Patrón Visitor

Cada vez que el parser identifica una regla exitosamente, crea instancias dinámicas (`new`) de clases derivadas de `Expr`, `Stmt` o `TopDecl` definidas en [ast.h].

Para facilitar que las fases subsecuentes del compilador (analizador semántico, optimizador y generador de código) recorran este árbol sin acoplarse a clases concretas, todos los nodos del AST heredan el método abstracto `accept(Visitor*)`:

```cpp
class Expr {
public:
    virtual ~Expr() = default;
    virtual void accept(Visitor* v) = 0;
};
```

Por ejemplo, un nodo `BinaryExpr` implementa este método redirigiendo el flujo al visitor:
```cpp
void BinaryExpr::accept(Visitor* v) override {
    v->visit(this);
}
```
Esto permite un recorrido limpio y modular del AST en las siguientes etapas del pipeline de compilación.
