#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include "token.h"

struct LexError : std::runtime_error {
    int line, col;
    LexError(const std::string& msg, int line, int col)
        : std::runtime_error(msg), line(line), col(col) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& source);

    Token              nextToken();   // siguiente token (lazy)
    std::vector<Token> tokenize();    // tokeniza todo de una vez

private:
    std::string source;
    int pos;
    int line;
    int col;

    char current() const;
    char peek(int offset = 1) const;
    char advance();

    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();

    Token readIdentifierOrKeyword();
    Token readNumber();
    Token readCharLiteral();
    Token readStringLiteral();
    Token readOperator();

    static TokenType keywordType(const std::string& word);
};
