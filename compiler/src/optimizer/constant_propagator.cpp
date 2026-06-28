#include "constant_propagator.h"
#include "../parser/ast.h"
#include <vector>

using LitVal = ConstantPropagator::LitVal;

// ─── Literales ───────────────────────────────────────────────────────────────

static bool fromLiteral(Expr* e, LitVal& v) {
    if (auto* i = dynamic_cast<IntLitExpr*>(e))   { v.kind = LitVal::Int;   v.i = i->value; return true; }
    if (auto* f = dynamic_cast<FloatLitExpr*>(e)) { v.kind = LitVal::Float; v.f = f->value; return true; }
    if (auto* b = dynamic_cast<BoolLitExpr*>(e))  { v.kind = LitVal::Bool;  v.b = b->value; return true; }
    if (auto* c = dynamic_cast<CharLitExpr*>(e))  { v.kind = LitVal::Char;  v.s = c->value; return true; }
    return false;
}

static Expr* makeLiteral(const LitVal& v) {
    switch (v.kind) {
        case LitVal::Int:   return new IntLitExpr(v.i);
        case LitVal::Float: return new FloatLitExpr(v.f);
        case LitVal::Bool:  return new BoolLitExpr(v.b);
        case LitVal::Char:  return new CharLitExpr(v.s);
    }
    return nullptr;
}

// Solo se propaga si el tipo del literal coincide con el tipo declarado de la
// variable. Si no (p.ej. `float x = 5`, con literal int), sustituir el literal
// crudo perdería la conversión implícita que el codegen aplica por el tipo de x.
static bool typeMatches(TypeNode* t, const LitVal& v) {
    if (!t || !t->mods.empty()) return false;  // sin tipo o puntero
    if (t->is_auto) return true;               // auto: el tipo es el del literal
    switch (v.kind) {
        case LitVal::Int:   return t->base == "int";
        case LitVal::Float: return t->base == "float";
        case LitVal::Bool:  return t->base == "bool";
        case LitVal::Char:  return t->base == "char";
    }
    return false;
}

// ─── Colectores auxiliares (recorridos de solo lectura) ──────────────────────

namespace {

// Reúne los nombres de variables que se MODIFICAN en un subárbol: asignación,
// ++/--, y toma de dirección. Sirve para invalidar constantes en control de flujo.
class AssignedCollector : public AstWalker {
public:
    explicit AssignedCollector(std::set<std::string>& out) : out_(out) {}

    void visit(AssignExpr* n) override {
        if (auto* id = dynamic_cast<IdExpr*>(n->left)) out_.insert(id->name);
        AstWalker::visit(n);
    }
    void visit(UnaryExpr* n) override {
        if (n->op == UnaryOp::AddrOf || n->op == UnaryOp::PreInc || n->op == UnaryOp::PreDec)
            if (auto* id = dynamic_cast<IdExpr*>(n->expr)) out_.insert(id->name);
        AstWalker::visit(n);
    }
    void visit(PostfixExpr* n) override {
        if (auto* id = dynamic_cast<IdExpr*>(n->base)) out_.insert(id->name);
        AstWalker::visit(n);
    }
    void visit(VarDeclStmt* n) override {
        out_.insert(n->name);  // declaración interna: no propagar el nombre externo
        AstWalker::visit(n);
    }

private:
    std::set<std::string>& out_;
};

// Reúne las variables cuya dirección se toma (&x) en un subárbol.
class AddrTakenCollector : public AstWalker {
public:
    explicit AddrTakenCollector(std::set<std::string>& out) : out_(out) {}

    void visit(UnaryExpr* n) override {
        if (n->op == UnaryOp::AddrOf)
            if (auto* id = dynamic_cast<IdExpr*>(n->expr)) out_.insert(id->name);
        AstWalker::visit(n);
    }

private:
    std::set<std::string>& out_;
};

void collectAssigned(Stmt* s, std::set<std::string>& out) {
    if (!s) return;
    AssignedCollector c(out);
    s->accept(&c);
}
void collectAssigned(Expr* e, std::set<std::string>& out) {
    if (!e) return;
    AssignedCollector c(out);
    e->accept(&c);
}

}  // namespace

// ─── Visitas ─────────────────────────────────────────────────────────────────

void ConstantPropagator::visit(FuncDecl* node) {
    consts_.clear();
    addrTaken_.clear();
    AddrTakenCollector c(addrTaken_);
    node->body->accept(&c);
    node->body->accept(this);
}

void ConstantPropagator::visit(Block* node) {
    // Flujo lineal: las constantes fluyen entre sentencias; al salir del bloque
    // se descartan las variables declaradas en él (fuera de alcance).
    std::vector<std::string> declared;
    for (Stmt* s : node->stmts) {
        s->accept(this);
        if (auto* vd = dynamic_cast<VarDeclStmt*>(s)) declared.push_back(vd->name);
    }
    for (const auto& n : declared) consts_.erase(n);
}

void ConstantPropagator::visit(IfStmt* node) {
    walk(node->condition);  // la condición corre con las constantes actuales

    auto saved = consts_;
    node->then_branch->accept(this);
    consts_ = saved;
    if (node->else_branch) {
        node->else_branch->accept(this);
        consts_ = saved;
    }

    // Tras el if, toda variable asignada en alguna rama queda incierta.
    std::set<std::string> mod;
    collectAssigned(node->then_branch, mod);
    if (node->else_branch) collectAssigned(node->else_branch, mod);
    for (const auto& v : mod) consts_.erase(v);
}

void ConstantPropagator::visit(WhileStmt* node) {
    // El cuerpo puede iterar: invalidar antes lo que se modifica dentro.
    std::set<std::string> mod;
    collectAssigned(node->condition, mod);
    collectAssigned(node->body, mod);
    for (const auto& v : mod) consts_.erase(v);

    walk(node->condition);
    auto saved = consts_;
    node->body->accept(this);
    consts_ = saved;
}

void ConstantPropagator::visit(ForStmt* node) {
    if      (node->init.decl) node->init.decl->accept(this);
    else if (node->init.expr) walk(node->init.expr);

    std::set<std::string> mod;
    collectAssigned(node->condition, mod);
    collectAssigned(node->update, mod);
    collectAssigned(node->body, mod);
    for (const auto& v : mod) consts_.erase(v);

    if (node->condition) walk(node->condition);
    auto saved = consts_;
    node->body->accept(this);
    consts_ = saved;
    if (node->update) walk(node->update);

    if (node->init.decl) consts_.erase(node->init.decl->name);
}

void ConstantPropagator::visit(VarDeclStmt* node) {
    walk(node->init);
    for (auto& d : node->dimensions) walk(d);
    for (auto& e : node->init_list)  walk(e);

    LitVal v;
    if (node->dimensions.empty() && node->init && !addrTaken_.count(node->name) &&
        fromLiteral(node->init, v) && typeMatches(node->type, v)) {
        consts_[node->name] = v;
    } else {
        consts_.erase(node->name);  // sombrea cualquier constante externa homónima
    }
}

void ConstantPropagator::visit(AssignExpr* node) {
    walk(node->right);  // propagar en el lado derecho (rvalue)

    if (auto* id = dynamic_cast<IdExpr*>(node->left)) {
        // No registramos constantes desde asignaciones: no conocemos aquí el tipo
        // declarado de la variable (necesario para no perder conversiones). La
        // variable simplemente deja de ser constante.
        consts_.erase(id->name);
    } else {
        walk(node->left);  // lvalue compuesto (arr[k], s.m): propagar sub-rvalues
    }
}

void ConstantPropagator::visit(UnaryExpr* node) {
    if (node->op == UnaryOp::AddrOf || node->op == UnaryOp::PreInc || node->op == UnaryOp::PreDec) {
        if (auto* id = dynamic_cast<IdExpr*>(node->expr)) consts_.erase(id->name);
        else walk(node->expr);
    } else {
        walk(node->expr);  // Neg / Not / Deref: el operando es rvalue
    }
}

void ConstantPropagator::visit(PostfixExpr* node) {
    if (auto* id = dynamic_cast<IdExpr*>(node->base)) consts_.erase(id->name);
    else walk(node->base);
}

void ConstantPropagator::visit(IdExpr* node) {
    auto it = consts_.find(node->name);
    if (it != consts_.end()) replaceWith(makeLiteral(it->second));
}
