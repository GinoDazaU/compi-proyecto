# Analizador Léxico (Lexer)

El Lexer (también conocido como **Scanner** o **Analizador Léxico**) es el encargado de masticar el archivo de texto plano caracter por caracter y agruparlos en los `Token` que acabamos de estudiar.

Vamos a analizar cómo está diseñado y cómo funciona cada parte en lexer.h y lexer.cpp.

---

### 1. La estructura de lexer.h

El archivo de cabecera declara una clase `Lexer` que mantiene el estado de la lectura:

```cpp
private:
    std::string source; // El código fuente completo cargado en memoria.
    int pos;            // Índice del caracter actual que estamos leyendo (0, 1, 2...).
    int line;           // Fila actual (empieza en 1).
    int col;            // Columna actual (empieza en 1).
```

#### Los 3 Helpers de Navegación Básica (El "motor" del Lexer)
Para movernos por el texto de manera segura, usamos tres métodos privados muy simples pero cruciales:
1. **`current()`**: Retorna el caracter en la posición `pos`. Si llegamos al final del archivo, retorna `\0` (caracter nulo).
2. **`peek(offset)`**: Permite "mirar hacia adelante" sin consumir el caracter. Por ejemplo, si estamos en un `+` y queremos saber si el siguiente caracter es otro `+` (para hacer `++`), llamamos a `peek()`.
3. **`advance()`**: Consume el caracter actual, avanza la posición `pos` e incrementa la columna `col`. Si el caracter consumido era un salto de línea (`\n`), reinicia `col = 1` e incrementa `line`. Retorna el caracter consumido.

---

### 2. El flujo principal en lexer.cpp

El punto de entrada cuando el compilador quiere el siguiente token es **`nextToken()`**:

```cpp
Token Lexer::nextToken() {
    // 1. Saltamos comentarios y espacios en blanco
    while (true) {
        skipWhitespace();
        if (current() == '/' && peek() == '/')      skipLineComment();
        else if (current() == '/' && peek() == '*') skipBlockComment();
        else break; // Si no hay comentarios ni espacios, paramos de saltar
    }

    // 2. ¿Llegamos al final del archivo?
    if (current() == '\0')
        return Token(TokenType::END, "", line, col);

    // 3. Identificar el tipo de token según el caracter inicial:
    if (std::isalpha(current()) || current() == '_') return readIdentifierOrKeyword();
    if (std::isdigit(current()))                     return readNumber();
    if (current() == '\'')                           return readCharLiteral();
    if (current() == '"')                            return readStringLiteral();

    // 4. Si no es ninguna de las anteriores, debe ser un operador o delimitador (o un error)
    return readOperator();
}
```

---

### 3. Las sub-funciones de lectura de tokens (Explicación paso a paso)

#### A. Identificadores y Palabras Reservadas (`readIdentifierOrKeyword`)
En C++, un identificador debe empezar con una letra o un guion bajo (`_`), y puede continuar con letras, números o `_`.
```cpp
Token Lexer::readIdentifierOrKeyword() {
    int startCol = col;
    std::string lexeme;
    while (std::isalnum(current()) || current() == '_')
        lexeme += advance(); // Acumula caracteres válidos
    
    // Al finalizar, chequeamos si este lexema es una palabra reservada (ej: "if", "int")
    // O si es un identificador cualquiera (ej: "contador")
    return Token(keywordType(lexeme), lexeme, line, startCol);
}
```
`keywordType` busca en el mapa estático `KEYWORDS`. Si lo encuentra, devuelve su `TokenType` específico (ej. `KW_IF`). Si no, devuelve `TokenType::ID`.

#### B. Números (`readNumber`)
Soporta números enteros (`42`) y decimales/punto flotante (`3.14`), así como notación científica (`1e-5`).
1. Acumula todos los dígitos iniciales.
2. Si encuentra un punto `.` seguido de otro dígito, sabe que es un flotante (`isFloat = true`) y acumula la parte decimal.
3. Si encuentra una `e` o `E`, procesa el exponente (incluyendo un opcional `+` o `-`).
4. Retorna `FLOAT_LIT` o `INT_LIT` según el flag `isFloat`.

#### C. Literales de Caracter (`readCharLiteral`)
Procesa cosas como `'a'` o secuencias de escape como `'\n'`.
* Empieza consumiendo `'`.
* Si encuentra una barra invertida `\`, consume el siguiente caracter (escape).
* Verifica que termine con `'`. Si falta el cierre, devuelve un token de tipo `ERR` (error léxico).

#### D. Cadenas de Texto (`readStringLiteral`)
Procesa strings como `"Hola Mundo"`.
* Lee caracteres dentro de las comillas dobles `"`.
* Si encuentra `\"`, la barra de escape `\` le permite incluir comillas dentro del string sin cerrarlo prematuramente.
* Al final consume la comilla de cierre.

#### E. Operadores (`readOperator`)
Este es un gran `switch-case` que implementa una pequeña máquina de estados para resolver la ambigüedad de operadores de múltiples caracteres. Por ejemplo, al leer `+`:
```cpp
case '+':
    if (n == '+') { advance(); return Token(TokenType::INC, "++", line, startCol); }
    if (n == '=') { advance(); return Token(TokenType::PLUS_ASSIGN, "+=", line, startCol); }
    return Token(TokenType::PLUS, "+", line, startCol);
```
* Si el siguiente caracter es `+`, avanza y retorna `INC` (`++`).
* Si es `=`, avanza y retorna `PLUS_ASSIGN` (`+=`).
* Si es cualquier otra cosa, no avanza más y retorna el simple `PLUS` (`+`).
* Hace lo mismo para operadores de comparación (`==`, `!=`, `<=`, `>=`), asignación compuesta y accesos a miembros (`->`).

---

### Resumen del flujo
* **`nextToken()`**: Es el método principal que utilizará el **Parser** para ir pidiendo tokens uno a uno a demanda (evaluación perezosa o lazy), permitiéndole validar la sintaxis y construir el Árbol de Sintaxis Abstracta (AST) de forma eficiente.
* **`tokenize()`**: Es un método helper que internamente ejecuta un bucle `while` llamando a `nextToken()` para recolectar todos los tokens del archivo en un vector (`std::vector<Token>`). Actualmente se utiliza en el punto de entrada principal (`main.cpp`) para depuración, y es de gran utilidad para realizar pruebas unitarias del lexer o para alimentar al frontend visualizador.
