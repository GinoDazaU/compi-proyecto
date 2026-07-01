#include "ast_walker.h"
#include "../parser/ast.h"

void AstWalker::run(Program* program) {
    if (program) program->accept(this);
}

// Visita el slot y, si el pase pidió un reemplazo, lo intercambia liberando el
// nodo viejo. Guarda/restaura repl_ para soportar la recursión anidada.
void AstWalker::walk(Expr*& slot) {
    if (!slot) return;
    Expr* saved = repl_;
    repl_ = nullptr;
    slot->accept(this);
    if (repl_ && repl_ != slot) {
        delete slot;
        slot = repl_;
    }
    repl_ = saved;
}

void AstWalker::visit(Program* node) {
    for (auto d : node->decls) d->accept(this);
}

// ─── Expresiones ─────────────────────────────────────────────────────────────
// Hojas: sin hijos que recorrer.
void AstWalker::visit(IntLitExpr*)    {}
void AstWalker::visit(FloatLitExpr*)  {}
void AstWalker::visit(BoolLitExpr*)   {}
void AstWalker::visit(CharLitExpr*)   {}
void AstWalker::visit(StringLitExpr*) {}
void AstWalker::visit(IdExpr*)        {}

void AstWalker::visit(BinaryExpr* node) { walk(node->left); walk(node->right); }
void AstWalker::visit(UnaryExpr* node)  { walk(node->expr); }
void AstWalker::visit(AssignExpr* node) { walk(node->left); walk(node->right); }
void AstWalker::visit(NewArrayExpr* node)  { walk(node->size); }
void AstWalker::visit(NewObjectExpr*)      {}
void AstWalker::visit(IndexExpr* node)  { walk(node->base); walk(node->index); }

void AstWalker::visit(CallExpr* node) {
    walk(node->callee);
    for (auto& a : node->args) walk(a);
}

void AstWalker::visit(MemberExpr* node)  { walk(node->base); }
void AstWalker::visit(PostfixExpr* node) { walk(node->base); }

// ─── Sentencias ──────────────────────────────────────────────────────────────
void AstWalker::visit(Block* node) {
    for (auto s : node->stmts) s->accept(this);
}

void AstWalker::visit(VarDeclStmt* node) {
    walk(node->init);
    for (auto& d : node->dimensions) walk(d);
    for (auto& e : node->init_list)  walk(e);
}

void AstWalker::visit(ExprStmt* node) { walk(node->expr); }

void AstWalker::visit(IfStmt* node) {
    walk(node->condition);
    node->then_branch->accept(this);
    if (node->else_branch) node->else_branch->accept(this);
}

void AstWalker::visit(WhileStmt* node) {
    walk(node->condition);
    node->body->accept(this);
}

void AstWalker::visit(ForStmt* node) {
    if      (node->init.decl) node->init.decl->accept(this);
    else if (node->init.expr) walk(node->init.expr);
    if (node->condition) walk(node->condition);
    if (node->update)    walk(node->update);
    node->body->accept(this);
}

void AstWalker::visit(ReturnStmt* node) { walk(node->expr); }
void AstWalker::visit(BreakStmt*)       {}
void AstWalker::visit(ContinueStmt*)    {}
void AstWalker::visit(DeleteStmt* node) { walk(node->expr); }

// ─── Declaraciones globales ──────────────────────────────────────────────────
void AstWalker::visit(StructDecl*)    {}
void AstWalker::visit(FuncDecl* node) { node->body->accept(this); }
