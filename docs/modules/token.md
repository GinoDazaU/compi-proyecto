### 1. ¿Qué es un Token?
En teoría de compiladores, un token es la unidad mínima con significado léxico. Por ejemplo, en la línea de código `int x = 42;`, los tokens son:

1. Un tipo de dato (`int` -> keyword)
2. Un nombre de variable (`x` -> identificador)
3. Un operador de asignación (`=` -> operador)
4. Un número literal (`42` -> literal entero)
5. Un delimitador (`;` -> punto y coma)

---

### 2. Estructura de token.h

El archivo de cabecera define dos cosas principales: un tipo enumerado (`enum class TokenType`) y la estructura `Token`.

#### A. `enum class TokenType`
Es un listado ordenado de todas las categorías posibles de tokens que el compilador entiende. Está agrupado en:
* **Literales**: `INT_LIT` (ej. `42`), `FLOAT_LIT` (ej. `3.14`), `CHAR_LIT` (ej. `'a'`), `STRING_LIT` (ej. `"hola"`).
* **Identificadores**: `ID` para nombres de variables, funciones o structs (ej. `mi_variable`, `calcular`).
* **Keywords (Palabras reservadas)**:
  * Tipos básicos (`KW_INT`, `KW_FLOAT`, `KW_BOOL`, etc.).
  * Estructuras y templates (`KW_STRUCT`, `KW_TEMPLATE`, `KW_TYPENAME`).
  * Flujo de control (`KW_IF`, `KW_ELSE`, `KW_WHILE`, `KW_FOR`, `KW_RETURN`, etc.).
* **Operadores**: Aritméticos (`PLUS`, `MINUS`, `STAR`...), Asignación (`ASSIGN`, `PLUS_ASSIGN`...), Comparación (`EQ`, `NEQ`...), Lógicos (`AND`, `OR`...) y Bit a bit.
* **Delimitadores**: Paréntesis (`LPAREN`, `RPAREN`), llaves (`LBRACE`, `RBRACE`), corchetes, comas y punto y coma.
* **Especiales**: 
  * `END`: Marca el fin del archivo (EOF - End Of File). Es clave para que el Parser sepa cuándo parar.
  * `ERR`: Representa un token inválido (caracteres extraños o strings sin cerrar).

#### B. La estructura `Token`
```cpp
struct Token {
    TokenType   type;       // Qué tipo de token es (ej. TokenType::KW_INT)
    std::string lexeme;     // El texto exacto como se escribió en el código (ej. "int")
    int         line;       // Fila donde inicia en el código fuente (para reportar errores)
    int         col;        // Columna donde inicia

    // ...
};
```
* **`type`**: Le sirve al parser para validar la estructura sintáctica del programa (ej. *¿después de un `if` viene un `(` ?*).
* **`lexeme`**: El contenido textual. Si el token es de tipo `ID`, necesitamos el `lexeme` para saber qué variable es (ej. `"x"` o `"y"`).
* **`line` y `col`**: **Esencial para la experiencia de usuario.** Si hay un error semántico o sintáctico en la línea 40, columna 12, usamos estos atributos para imprimir mensajes claros como:
  > `Error en la línea 40, col 12: La variable 'x' no está declarada.`

---

### 3. Implementación en token.cpp

El archivo de código implementa el comportamiento asociado a los tokens:

#### A. Constructor
```cpp
Token::Token(TokenType type, std::string lexeme, int line, int col)
    : type(type), lexeme(std::move(lexeme)), line(line), col(col) {}
```
Inicializa todos los campos de forma sencilla y eficiente (`std::move` evita copias innecesarias del texto del lexema).

#### B. `typeName()`
Traduce los valores del enum (`TokenType::KW_INT`) a cadenas legibles (`"KW_INT"`). 
* Esto es sumamente útil a la hora de depurar (debugging), porque imprimir un número entero que representa al enum es difícil de leer. Imprimir `"KW_INT"` es inmediato de comprender.

#### C. Sobrecarga del operador `<<`
```cpp
std::ostream& operator<<(std::ostream& out, const Token& tok) {
    return out << tok.typeName()
               << "(\"" << tok.lexeme << "\", "
               << tok.line << ":" << tok.col << ")";
}
```
Esto te permite escribir cosas como `std::cout << mi_token << std::endl;` y ver en consola un formato limpio:
> `KW_INT("int", 1:1)`
> `ID("x", 1:5)`
> `ASSIGN("=", 1:7)`
> `INT_LIT("42", 1:9)`

---

### 4. ¿Cómo interactúa esto con el Lexer?

Si observamos el lexer.cpp:
1. El lexer lee caracteres secuenciales del código fuente.
2. Identifica patrones usando funciones como `readNumber()`, `readIdentifierOrKeyword()`, `readOperator()`.
3. Para palabras que parecen variables (ej. `int` o `x`), busca en una tabla hash llamada `KEYWORDS` (línea 7 de `lexer.cpp`):
   - Si la palabra está en `KEYWORDS` (ej. `"int"`), crea un `Token` con el tipo correspondiente (`TokenType::KW_INT`).
   - Si no está (ej. `"x"` o `"mi_funcion"`), asume que es un identificador definido por el usuario y le asigna `TokenType::ID`.
4. El parser irá llamando recursivamente a `lexer.nextToken()` para recibir uno por uno estos tokens e ir armando el Árbol de Sintaxis Abstracta (AST).
