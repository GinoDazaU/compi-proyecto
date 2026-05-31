#pragma once
#include "ast.h"
#include <iostream>
#include <string>

class ASTPrinter : public Visitor {
public:
    explicit ASTPrinter(std::ostream& out = std::cout) : out(out) {}

    void visit(Program* node)          override;
    void visit(IntLitExpr* node)       override;
    void visit(FloatLitExpr* node)     override;
    void visit(BoolLitExpr* node)      override;
    void visit(CharLitExpr* node)      override;
    void visit(StringLitExpr* node)    override;
    void visit(IdExpr* node)           override;
    void visit(BinaryExpr* node)       override;
    void visit(UnaryExpr* node)        override;
    void visit(AssignExpr* node)       override;
    void visit(CastExpr* node)         override;
    void visit(NewArrayExpr* node)     override;
    void visit(NewObjectExpr* node)    override;
    void visit(IndexExpr* node)        override;
    void visit(CallExpr* node)         override;
    void visit(MemberExpr* node)       override;
    void visit(PostfixExpr* node)      override;
    void visit(LambdaExpr* node)       override;
    void visit(Block* node)            override;
    void visit(VarDeclStmt* node)      override;
    void visit(ExprStmt* node)         override;
    void visit(IfStmt* node)           override;
    void visit(WhileStmt* node)        override;
    void visit(ForStmt* node)          override;
    void visit(ForRangeStmt* node)     override;
    void visit(ReturnStmt* node)       override;
    void visit(BreakStmt* node)        override;
    void visit(ContinueStmt* node)     override;
    void visit(DeleteStmt* node)       override;
    void visit(GlobalVarDecl* node)    override;
    void visit(StructDecl* node)       override;
    void visit(FuncDecl* node)         override;
    void visit(TemplateFuncDecl* node) override;

private:
    std::ostream& out;
    int depth = 0;

    void indent();
    std::string typeStr(TypeNode* t);
    static const char* binaryOpStr(BinaryOp op);
    static const char* assignOpStr(AssignOp op);
    static const char* unaryOpStr(UnaryOp op);
};
