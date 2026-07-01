#pragma once
#include "../parser/visitor.h"

class Expr;

// ─── AstWalker ───────────────────────────────────────────────────────────────
// Recorrido por defecto del AST: implementa todas las visit() descendiendo a los
// hijos sin hacer nada más. Cada pase de optimización hereda de aquí y solo
// sobreescribe los nodos que le interesan.
//
// Para reescribir el árbol, un pase llama replaceWith(nuevo) dentro de su visit;
// el AstWalker, al recorrer cada slot de expresión, intercambia el puntero del
// padre y libera el nodo viejo. Esto resuelve el problema de que Visitor::visit
// devuelva void (no puede "retornar" el nodo reemplazado).
class AstWalker : public Visitor {
public:
    // Punto de entrada: recorre todo el programa.
    void run(Program* program);

protected:
    // Un pase llama esto en su visit para sustituir la expresión actual.
    void replaceWith(Expr* e) { repl_ = e; }

    // Visita un slot de expresión y aplica el reemplazo pedido (si lo hubo).
    void walk(Expr*& slot);

public:
    void visit(Program* node) override;

    // Expresiones
    void visit(IntLitExpr* node) override;
    void visit(FloatLitExpr* node) override;
    void visit(BoolLitExpr* node) override;
    void visit(CharLitExpr* node) override;
    void visit(StringLitExpr* node) override;
    void visit(IdExpr* node) override;
    void visit(BinaryExpr* node) override;
    void visit(UnaryExpr* node) override;
    void visit(AssignExpr* node) override;
    void visit(NewArrayExpr* node) override;
    void visit(NewObjectExpr* node) override;
    void visit(IndexExpr* node) override;
    void visit(CallExpr* node) override;
    void visit(MemberExpr* node) override;
    void visit(PostfixExpr* node) override;

    // Sentencias
    void visit(Block* node) override;
    void visit(VarDeclStmt* node) override;
    void visit(ExprStmt* node) override;
    void visit(IfStmt* node) override;
    void visit(WhileStmt* node) override;
    void visit(ForStmt* node) override;
    void visit(ReturnStmt* node) override;
    void visit(BreakStmt* node) override;
    void visit(ContinueStmt* node) override;
    void visit(DeleteStmt* node) override;

    // Declaraciones globales
    void visit(StructDecl* node) override;
    void visit(FuncDecl* node) override;

private:
    Expr* repl_ = nullptr;  // reemplazo pendiente para el slot que se visita
};
