#pragma once

// Forward declarations de las clases del AST
class Program;

// Expresiones
class IntLitExpr;
class FloatLitExpr;
class BoolLitExpr;
class CharLitExpr;
class StringLitExpr;
class IdExpr;
class BinaryExpr;
class UnaryExpr;
class AssignExpr;
class CastExpr;
class NewArrayExpr;
class NewObjectExpr;
class IndexExpr;
class CallExpr;
class MemberExpr;
class PostfixExpr;
class LambdaExpr;

// Sentencias
class Block;
class VarDeclStmt;
class ExprStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class ForRangeStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;
class DeleteStmt;

// Declaraciones Globales
class GlobalVarDecl;
class StructDecl;
class FuncDecl;
class TemplateFuncDecl;

class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit(Program* node) = 0;

    // Expresiones
    virtual void visit(IntLitExpr* node) = 0;
    virtual void visit(FloatLitExpr* node) = 0;
    virtual void visit(BoolLitExpr* node) = 0;
    virtual void visit(CharLitExpr* node) = 0;
    virtual void visit(StringLitExpr* node) = 0;
    virtual void visit(IdExpr* node) = 0;
    virtual void visit(BinaryExpr* node) = 0;
    virtual void visit(UnaryExpr* node) = 0;
    virtual void visit(AssignExpr* node) = 0;
    virtual void visit(CastExpr* node) = 0;
    virtual void visit(NewArrayExpr* node) = 0;
    virtual void visit(NewObjectExpr* node) = 0;
    virtual void visit(IndexExpr* node) = 0;
    virtual void visit(CallExpr* node) = 0;
    virtual void visit(MemberExpr* node) = 0;
    virtual void visit(PostfixExpr* node) = 0;
    virtual void visit(LambdaExpr* node) = 0;

    // Sentencias
    virtual void visit(Block* node) = 0;
    virtual void visit(VarDeclStmt* node) = 0;
    virtual void visit(ExprStmt* node) = 0;
    virtual void visit(IfStmt* node) = 0;
    virtual void visit(WhileStmt* node) = 0;
    virtual void visit(ForStmt* node) = 0;
    virtual void visit(ForRangeStmt* node) = 0;
    virtual void visit(ReturnStmt* node) = 0;
    virtual void visit(BreakStmt* node) = 0;
    virtual void visit(ContinueStmt* node) = 0;
    virtual void visit(DeleteStmt* node) = 0;

    // Declaraciones Globales
    virtual void visit(GlobalVarDecl* node) = 0;
    virtual void visit(StructDecl* node) = 0;
    virtual void visit(FuncDecl* node) = 0;
    virtual void visit(TemplateFuncDecl* node) = 0;
};