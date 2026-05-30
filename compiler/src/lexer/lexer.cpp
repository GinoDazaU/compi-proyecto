#include "lexer.h"
#include <cctype>
#include <unordered_map>

// ─── Tabla de keywords ────────────────────────────────────────────────────────

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"int",         TokenType::KW_INT},
    {"long",        TokenType::KW_LONG},
    {"float",       TokenType::KW_FLOAT},
    {"double",      TokenType::KW_DOUBLE},
    {"bool",        TokenType::KW_BOOL},
    {"char",        TokenType::KW_CHAR},
    {"void",        TokenType::KW_VOID},
    {"auto",        TokenType::KW_AUTO},
    {"string",      TokenType::KW_STRING},
    {"const",       TokenType::KW_CONST},
    {"struct",      TokenType::KW_STRUCT},
    {"template",    TokenType::KW_TEMPLATE},
    {"typename",    TokenType::KW_TYPENAME},
    {"new",         TokenType::KW_NEW},
    {"delete",      TokenType::KW_DELETE},
    {"if",          TokenType::KW_IF},
    {"else",        TokenType::KW_ELSE},
    {"while",       TokenType::KW_WHILE},
    {"for",         TokenType::KW_FOR},
    {"break",       TokenType::KW_BREAK},
    {"continue",    TokenType::KW_CONTINUE},
    {"return",      TokenType::KW_RETURN},
    {"true",        TokenType::KW_TRUE},
    {"false",       TokenType::KW_FALSE},
    {"static_cast", TokenType::KW_STATIC_CAST},
};

// ─── Constructor ──────────────────────────────────────────────────────────────

Lexer::Lexer(const std::string& source)
    : source(source), pos(0), line(1), col(1) {}

// ─── Helpers de navegación ────────────────────────────────────────────────────

char Lexer::current() const {
    return pos < (int)source.size() ? source[pos] : '\0';
}

char Lexer::peek(int offset) const {
    int p = pos + offset;
    return p < (int)source.size() ? source[p] : '\0';
}

char Lexer::advance() {
    char c = current();
    ++pos;
    if (c == '\n') { ++line; col = 1; }
    else           { ++col; }
    return c;
}

// ─── Salto de blancos y comentarios ───────────────────────────────────────────

void Lexer::skipWhitespace() {
    while (std::isspace(current()))
        advance();
}

void Lexer::skipLineComment() {
    while (current() != '\n' && current() != '\0')
        advance();
}

void Lexer::skipBlockComment() {
    advance(); advance(); // consume /*
    while (current() != '\0') {
        if (current() == '*' && peek() == '/') {
            advance(); advance(); // consume */
            return;
        }
        advance();
    }
    // comentario de bloque sin cerrar — el error lo reportará el parser
}

// ─── Lectores de tokens ───────────────────────────────────────────────────────

TokenType Lexer::keywordType(const std::string& word) {
    auto it = KEYWORDS.find(word);
    return it != KEYWORDS.end() ? it->second : TokenType::ID;
}

Token Lexer::readIdentifierOrKeyword() {
    int startCol = col;
    std::string lexeme;
    while (std::isalnum(current()) || current() == '_')
        lexeme += advance();
    return Token(keywordType(lexeme), lexeme, line, startCol);
}

Token Lexer::readNumber() {
    int startCol = col;
    std::string lexeme;
    bool isFloat = false;

    while (std::isdigit(current()))
        lexeme += advance();

    if (current() == '.' && std::isdigit(peek())) {
        isFloat = true;
        lexeme += advance();
        while (std::isdigit(current()))
            lexeme += advance();
    }

    if (current() == 'e' || current() == 'E') {
        isFloat = true;
        lexeme += advance();
        if (current() == '+' || current() == '-')
            lexeme += advance();
        while (std::isdigit(current()))
            lexeme += advance();
    }

    return Token(isFloat ? TokenType::FLOAT_LIT : TokenType::INT_LIT, lexeme, line, startCol);
}

Token Lexer::readCharLiteral() {
    int startCol = col;
    std::string lexeme;
    lexeme += advance(); // '
    if (current() == '\\') {
        lexeme += advance();
        lexeme += advance();
    } else {
        lexeme += advance();
    }
    if (current() == '\'') lexeme += advance();
    return Token(TokenType::CHAR_LIT, lexeme, line, startCol);
}

Token Lexer::readStringLiteral() {
    int startCol = col;
    std::string lexeme;
    lexeme += advance(); // "
    while (current() != '"' && current() != '\0') {
        if (current() == '\\') lexeme += advance(); // escape
        lexeme += advance();
    }
    if (current() == '"') lexeme += advance();
    return Token(TokenType::STRING_LIT, lexeme, line, startCol);
}

Token Lexer::readOperator() {
    int startCol = col;
    char c = advance();
    char n = current();

    switch (c) {
        case '+':
            if (n == '+') { advance(); return Token(TokenType::INC,            "++", line, startCol); }
            if (n == '=') { advance(); return Token(TokenType::PLUS_ASSIGN,    "+=", line, startCol); }
            return Token(TokenType::PLUS,    "+", line, startCol);
        case '-':
            if (n == '-') { advance(); return Token(TokenType::DEC,            "--", line, startCol); }
            if (n == '=') { advance(); return Token(TokenType::MINUS_ASSIGN,   "-=", line, startCol); }
            if (n == '>') { advance(); return Token(TokenType::ARROW,          "->", line, startCol); }
            return Token(TokenType::MINUS,   "-", line, startCol);
        case '*':
            if (n == '=') { advance(); return Token(TokenType::STAR_ASSIGN,    "*=", line, startCol); }
            return Token(TokenType::STAR,    "*", line, startCol);
        case '/':
            if (n == '=') { advance(); return Token(TokenType::SLASH_ASSIGN,   "/=", line, startCol); }
            return Token(TokenType::SLASH,   "/", line, startCol);
        case '%':
            if (n == '=') { advance(); return Token(TokenType::PERCENT_ASSIGN, "%=", line, startCol); }
            return Token(TokenType::PERCENT, "%", line, startCol);
        case '=':
            if (n == '=') { advance(); return Token(TokenType::EQ,             "==", line, startCol); }
            return Token(TokenType::ASSIGN,  "=", line, startCol);
        case '!':
            if (n == '=') { advance(); return Token(TokenType::NEQ,            "!=", line, startCol); }
            return Token(TokenType::NOT,     "!", line, startCol);
        case '<':
            if (n == '=') { advance(); return Token(TokenType::LEQ,            "<=", line, startCol); }
            return Token(TokenType::LT,      "<", line, startCol);
        case '>':
            if (n == '=') { advance(); return Token(TokenType::GEQ,            ">=", line, startCol); }
            return Token(TokenType::GT,      ">", line, startCol);
        case '&':
            if (n == '&') { advance(); return Token(TokenType::AND,            "&&", line, startCol); }
            if (n == '=') { advance(); return Token(TokenType::AMP_ASSIGN,     "&=", line, startCol); }
            return Token(TokenType::AMP,     "&", line, startCol);
        case '|':
            if (n == '|') { advance(); return Token(TokenType::OR,             "||", line, startCol); }
            if (n == '=') { advance(); return Token(TokenType::PIPE_ASSIGN,    "|=", line, startCol); }
            return Token(TokenType::PIPE,    "|", line, startCol);
        case '~': return Token(TokenType::TILDE,     "~",  line, startCol);
        case '.': return Token(TokenType::DOT,       ".",  line, startCol);
        case '(': return Token(TokenType::LPAREN,    "(",  line, startCol);
        case ')': return Token(TokenType::RPAREN,    ")",  line, startCol);
        case '{': return Token(TokenType::LBRACE,    "{",  line, startCol);
        case '}': return Token(TokenType::RBRACE,    "}",  line, startCol);
        case '[': return Token(TokenType::LBRACKET,  "[",  line, startCol);
        case ']': return Token(TokenType::RBRACKET,  "]",  line, startCol);
        case ';': return Token(TokenType::SEMICOLON, ";",  line, startCol);
        case ',': return Token(TokenType::COMMA,     ",",  line, startCol);
        case ':': return Token(TokenType::COLON,     ":",  line, startCol);
        default:  return Token(TokenType::ERR, std::string(1, c), line, startCol);
    }
}

// ─── Interfaz pública ─────────────────────────────────────────────────────────

Token Lexer::nextToken() {
    while (true) {
        skipWhitespace();
        if (current() == '/' && peek() == '/')      skipLineComment();
        else if (current() == '/' && peek() == '*') skipBlockComment();
        else break;
    }

    if (current() == '\0')
        return Token(TokenType::END, "", line, col);

    if (std::isalpha(current()) || current() == '_') return readIdentifierOrKeyword();
    if (std::isdigit(current()))                     return readNumber();
    if (current() == '\'')                           return readCharLiteral();
    if (current() == '"')                            return readStringLiteral();

    return readOperator();
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.type == TokenType::END) break;
    }
    return tokens;
}
