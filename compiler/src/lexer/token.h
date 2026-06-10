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

    // Keywords — modificadores
    KW_CONST,

    // Keywords — estructuras y templates
    KW_STRUCT,
    KW_TEMPLATE,
    KW_TYPENAME,

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

    // Keywords — conversión de tipos
    KW_STATIC_CAST,

    // Operadores aritméticos
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /
    PERCENT,    // %

    // Operadores de asignación
    ASSIGN,         // =
    PLUS_ASSIGN,    // +=
    MINUS_ASSIGN,   // -=
    STAR_ASSIGN,    // *=
    SLASH_ASSIGN,   // /=
    PERCENT_ASSIGN, // %=
    AMP_ASSIGN,     // &=
    PIPE_ASSIGN,    // |=

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

    // Operadores bit a bit
    AMP,    // &
    PIPE,   // |
    TILDE,  // ~

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
    COLON,     // :

    // Especiales
    END,  // fin de entrada
    ERR   // token inválido
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
