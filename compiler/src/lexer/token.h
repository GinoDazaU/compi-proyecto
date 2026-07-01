#pragma once

#include <string>
#include <ostream>

enum class TokenType {
    // Literales
    INT_LIT,
    FLOAT_LIT,
    CHAR_LIT,
    STRING_LIT,

    // Identificador
    ID,

    // Keywords — tipos básicos
    KW_INT,
    KW_FLOAT,
    KW_BOOL,
    KW_CHAR,
    KW_VOID,
    KW_AUTO,
    KW_STRING,

    // Keywords — estructuras
    KW_STRUCT,

    // Keywords — memoria dinámica
    KW_NEW,
    KW_DELETE,

    // Keywords — control de flujo
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FOR,
    KW_BREAK,
    KW_CONTINUE,
    KW_RETURN,

    // Keywords — valores literales de tipo
    KW_TRUE,
    KW_FALSE,

    // Operadores aritméticos
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /
    PERCENT,    // %

    // Operador de asignación
    ASSIGN,         // =

    // Operadores de comparación
    EQ,   // ==
    NEQ,  // !=
    LT,   // <
    GT,   // >
    LEQ,  // <=
    GEQ,  // >=

    // Operadores lógicos
    AND,  // &&
    OR,   // ||
    NOT,  // !

    // Operador de dirección (address-of)
    AMP,    // &

    // Incremento y decremento
    INC,  // ++
    DEC,  // --

    // Acceso a miembros
    DOT,    // .
    ARROW,  // ->

    // Delimitadores
    LPAREN,    // (
    RPAREN,    // )
    LBRACE,    // {
    RBRACE,    // }
    LBRACKET,  // [
    RBRACKET,  // ]
    SEMICOLON, // ;
    COMMA,     // ,

    // Especiales
    END   // fin de entrada
};

struct Token {
    TokenType   type;
    std::string lexeme;
    int         line;
    int         col;

    Token(TokenType type, std::string lexeme, int line, int col);

    // Devuelve el nombre del tipo como string (útil para debug y errores)
    const char* typeName() const;

    friend std::ostream& operator<<(std::ostream& out, const Token& tok);
};
