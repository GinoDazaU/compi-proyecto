#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include "../lexer/token.h"
#include "ast.h"

struct ParseError : std::runtime_error {
    int line, col;
    ParseError(const std::string& msg, int line, int col)
        : std::runtime_error(msg), line(line), col(col) {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    Program* parse();

private:
    std::vector<Token> tokens;
    size_t pos = 0;

    // Navigation
    Token& cur();
    Token& peek(int offset = 1);
    Token  consume();
    Token  expect(TokenType t);
    bool   check(TokenType t);
    bool   match(TokenType t);
    [[noreturn]] void error(const std::string& msg);

    // Lookahead helpers
    bool isTypeStart();
    bool isRangeFor();

    // Types
    TypeNode* parseType();

    // Top-level declarations
    TopDecl*           parseTopDecl();
    StructDecl*        parseStructDecl();
    TemplateFuncDecl*  parseTemplateFuncDecl();
    FuncDecl*          parseFuncDecl(TypeNode* ret, std::string name);
    GlobalVarDecl*     parseGlobalVarDecl(TypeNode* type, std::string name);
    std::vector<Param> parseParamList();
    Param              parseParam();

    // Statements
    Block*       parseBlock();
    Stmt*        parseStmt();
    VarDeclStmt* parseVarDeclStmt(TypeNode* type, std::string name);
    IfStmt*      parseIfStmt();
    WhileStmt*   parseWhileStmt();
    Stmt*        parseForStmt();
    ReturnStmt*  parseReturnStmt();
    DeleteStmt*  parseDeleteStmt();

    // Expressions (de menor a mayor precedencia)
    Expr* parseExpr();
    Expr* parseAssign();
    Expr* parseLogicOr();
    Expr* parseLogicAnd();
    Expr* parseEquality();
    Expr* parseRelat();
    Expr* parseAdd();
    Expr* parseMul();
    Expr* parseUnary();
    Expr* parsePostfix();
    Expr* parsePrimary();
    LambdaExpr* parseLambda();

    std::vector<Expr*> parseArgList();
    std::vector<Expr*> parseInitList();
};
