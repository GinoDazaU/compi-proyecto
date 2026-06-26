#pragma once
#include <string>
#include <vector>
#include "visitor.h"

// ─── Forward declarations ──────────────────────────────────────────────────
struct TypeNode;
class Expr;
class Stmt;
class Block;
class TopDecl;
class FuncDecl;
class VarDeclStmt;

// ─── Tipos ────────────────────────────────────────────────────────────────
// PtrMod: * (trailing modifiers de un tipo)
enum class PtrMod { Pointer };

// Representa cualquier tipo del lenguaje, incluyendo const, punteros y templates
struct TypeNode {
    bool                is_auto       = false;
    std::string         base;           // "int", "float", "void", id definido por usuario
    TypeNode*           template_arg   = nullptr;  // para id<Type>
    std::vector<PtrMod> mods;           // modificadores en orden: *

    ~TypeNode() { delete template_arg; }
};

// ─── Enums de operadores ───────────────────────────────────────────────────
enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,
    Eq, Neq, Lt, Gt, Leq, Geq,
    And, Or
};

enum class UnaryOp {
    Neg,    // -
    Not,    // !
    Deref,  // *
    AddrOf, // &
    PreInc, // ++
    PreDec  // --
};

// ─── Clases base ───────────────────────────────────────────────────────────
class Expr {
public:
    int line = 0, col = 0;
    virtual ~Expr() = default;
    virtual void accept(Visitor* v) = 0;
};

class Stmt {
public:
    int line = 0, col = 0;
    virtual ~Stmt() = default;
    virtual void accept(Visitor* v) = 0;
};

class TopDecl {
public:
    int line = 0, col = 0;
    virtual ~TopDecl() = default;
    virtual void accept(Visitor* v) = 0;
};

// ─── Block (definido temprano porque LambdaExpr lo necesita) ──────────────
class Block : public Stmt {
public:
    std::vector<Stmt*> stmts;
    explicit Block(std::vector<Stmt*> s) : stmts(std::move(s)) {}
    ~Block() override { for (auto s : stmts) delete s; }
    void accept(Visitor* v) override { v->visit(this); }
};

// ─── Param (compartido por FuncDecl y LambdaExpr) ─────────────────────────
struct Param {
    TypeNode*   type;
    std::string name;
};


// ═══════════════════════════════════════════════════════════════════════════
// EXPRESIONES
// ═══════════════════════════════════════════════════════════════════════════

// Literales
class IntLitExpr : public Expr {
public:
    long long value;
    explicit IntLitExpr(long long v) : value(v) {}
    void accept(Visitor* v) override { v->visit(this); }
};

class FloatLitExpr : public Expr {
public:
    double value;
    explicit FloatLitExpr(double v) : value(v) {}
    void accept(Visitor* v) override { v->visit(this); }
};

class BoolLitExpr : public Expr {
public:
    bool value;
    explicit BoolLitExpr(bool v) : value(v) {}
    void accept(Visitor* v) override { v->visit(this); }
};

class CharLitExpr : public Expr {
public:
    std::string value;
    explicit CharLitExpr(std::string v) : value(std::move(v)) {}
    void accept(Visitor* v) override { v->visit(this); }
};

class StringLitExpr : public Expr {
public:
    std::string value;
    explicit StringLitExpr(std::string v) : value(std::move(v)) {}
    void accept(Visitor* v) override { v->visit(this); }
};

// Identificador
class IdExpr : public Expr {
public:
    std::string name;
    explicit IdExpr(std::string n) : name(std::move(n)) {}
    void accept(Visitor* v) override { v->visit(this); }
};

// Operadores binarios: +, -, *, /, %, ==, !=, <, >, <=, >=, &&, ||
class BinaryExpr : public Expr {
public:
    Expr*    left;
    BinaryOp op;
    Expr*    right;
    BinaryExpr(Expr* l, BinaryOp o, Expr* r) : left(l), op(o), right(r) {}
    ~BinaryExpr() override { delete left; delete right; }
    void accept(Visitor* v) override { v->visit(this); }
};

// Operadores unarios prefijos: -, !, *, &, ++, --
class UnaryExpr : public Expr {
public:
    UnaryOp op;
    Expr*   expr;
    UnaryExpr(UnaryOp o, Expr* e) : op(o), expr(e) {}
    ~UnaryExpr() override { delete expr; }
    void accept(Visitor* v) override { v->visit(this); }
};

// Asignación: =
class AssignExpr : public Expr {
public:
    Expr* left;
    Expr* right;
    AssignExpr(Expr* l, Expr* r) : left(l), right(r) {}
    ~AssignExpr() override { delete left; delete right; }
    void accept(Visitor* v) override { v->visit(this); }
};

// new Type[Expr]
class NewArrayExpr : public Expr {
public:
    TypeNode* type;
    Expr*     size;
    NewArrayExpr(TypeNode* t, Expr* s) : type(t), size(s) {}
    ~NewArrayExpr() override { delete type; delete size; }
    void accept(Visitor* v) override { v->visit(this); }
};

// new Type  (objeto dinámico, campos en cero)
class NewObjectExpr : public Expr {
public:
    TypeNode* type;
    explicit NewObjectExpr(TypeNode* t) : type(t) {}
    ~NewObjectExpr() override { delete type; }
    void accept(Visitor* v) override { v->visit(this); }
};

// Postfix: base[index]
class IndexExpr : public Expr {
public:
    Expr* base;
    Expr* index;
    IndexExpr(Expr* b, Expr* i) : base(b), index(i) {}
    ~IndexExpr() override { delete base; delete index; }
    void accept(Visitor* v) override { v->visit(this); }
};

// Postfix: callee(args)
class CallExpr : public Expr {
public:
    Expr*              callee;
    std::vector<Expr*> args;
    CallExpr(Expr* c, std::vector<Expr*> a) : callee(c), args(std::move(a)) {}
    ~CallExpr() override { delete callee; for (auto e : args) delete e; }
    void accept(Visitor* v) override { v->visit(this); }
};

// Postfix: base.member  o  base->member
class MemberExpr : public Expr {
public:
    Expr*       base;
    std::string member;
    bool        is_arrow;   // true = ->, false = .
    MemberExpr(Expr* b, std::string m, bool arrow)
        : base(b), member(std::move(m)), is_arrow(arrow) {}
    ~MemberExpr() override { delete base; }
    void accept(Visitor* v) override { v->visit(this); }
};

// Postfix: base++  o  base--
class PostfixExpr : public Expr {
public:
    Expr* base;
    bool  is_inc;           // true = ++, false = --
    PostfixExpr(Expr* b, bool inc) : base(b), is_inc(inc) {}
    ~PostfixExpr() override { delete base; }
    void accept(Visitor* v) override { v->visit(this); }
};



// [ ] ( ParamList ) [-> Type] Block  (lambdas sin capturas)
class LambdaExpr : public Expr {
public:
    std::vector<Param> params;
    TypeNode*          return_type;  // nullptr si no hay ->
    Block*             body;
    LambdaExpr(std::vector<Param> p, TypeNode* rt, Block* b)
        : params(std::move(p)), return_type(rt), body(b) {}
    ~LambdaExpr() override {
        for (auto& p : params) { delete p.type; }
        delete return_type;
        delete body;
    }
    void accept(Visitor* v) override { v->visit(this); }
};


// ═══════════════════════════════════════════════════════════════════════════
// SENTENCIAS
// ═══════════════════════════════════════════════════════════════════════════

// Type id [= Expr] ;
// Type id[Expr]([Expr])* [= { InitList }] ;
class VarDeclStmt : public Stmt {
public:
    TypeNode*          type;
    std::string        name;
    Expr*              init       = nullptr;  // init para variable simple
    std::vector<Expr*> dimensions;             // dimensiones si es array
    std::vector<Expr*> init_list;              // lista init de array
    VarDeclStmt(TypeNode* t, std::string n)
        : type(t), name(std::move(n)) {}
    ~VarDeclStmt() override {
        delete type; delete init;
        for (auto e : dimensions) delete e;
        for (auto e : init_list)  delete e;
    }
    void accept(Visitor* v) override { v->visit(this); }
};

class ExprStmt : public Stmt {
public:
    Expr* expr;
    explicit ExprStmt(Expr* e) : expr(e) {}
    ~ExprStmt() override { delete expr; }
    void accept(Visitor* v) override { v->visit(this); }
};

// if (Expr) Block [else (Block | IfStmt)]
class IfStmt : public Stmt {
public:
    Expr*  condition;
    Block* then_branch;
    Stmt*  else_branch;  // nullptr si no hay else (puede ser Block* o IfStmt*)
    IfStmt(Expr* c, Block* t, Stmt* e)
        : condition(c), then_branch(t), else_branch(e) {}
    ~IfStmt() override { delete condition; delete then_branch; delete else_branch; }
    void accept(Visitor* v) override { v->visit(this); }
};

// while (Expr) Block
class WhileStmt : public Stmt {
public:
    Expr*  condition;
    Block* body;
    WhileStmt(Expr* c, Block* b) : condition(c), body(b) {}
    ~WhileStmt() override { delete condition; delete body; }
    void accept(Visitor* v) override { v->visit(this); }
};

// for (ForInit ; Expr ; Expr) Block  —  ForInit es VarDecl, Expr, o vacío
struct ForInit {
    VarDeclStmt* decl = nullptr;
    Expr*        expr = nullptr;
    // ambos null → vacío
};

class ForStmt : public Stmt {
public:
    ForInit init;
    Expr*   condition;  // nullptr si omitida
    Expr*   update;     // nullptr si omitida
    Block*  body;
    ForStmt(ForInit i, Expr* c, Expr* u, Block* b)
        : init(i), condition(c), update(u), body(b) {}
    ~ForStmt() override {
        delete init.decl; delete init.expr;
        delete condition; delete update; delete body;
    }
    void accept(Visitor* v) override { v->visit(this); }
};

// return [Expr] ;
class ReturnStmt : public Stmt {
public:
    Expr* expr;  // nullptr si return void
    explicit ReturnStmt(Expr* e) : expr(e) {}
    ~ReturnStmt() override { delete expr; }
    void accept(Visitor* v) override { v->visit(this); }
};

class BreakStmt : public Stmt {
public:
    BreakStmt() = default;
    void accept(Visitor* v) override { v->visit(this); }
};

class ContinueStmt : public Stmt {
public:
    ContinueStmt() = default;
    void accept(Visitor* v) override { v->visit(this); }
};

// delete [[ ]] Expr ;
class DeleteStmt : public Stmt {
public:
    bool  is_array;
    Expr* expr;
    DeleteStmt(bool arr, Expr* e) : is_array(arr), expr(e) {}
    ~DeleteStmt() override { delete expr; }
    void accept(Visitor* v) override { v->visit(this); }
};


// ═══════════════════════════════════════════════════════════════════════════
// DECLARACIONES GLOBALES
// ═══════════════════════════════════════════════════════════════════════════

// struct id { MemberDecl* } ;
struct MemberDecl {
    TypeNode*   type;
    std::string name;
};

class StructDecl : public TopDecl {
public:
    std::string             name;
    std::vector<MemberDecl> members;
    StructDecl(std::string n, std::vector<MemberDecl> m)
        : name(std::move(n)), members(std::move(m)) {}
    ~StructDecl() override { for (auto& m : members) delete m.type; }
    void accept(Visitor* v) override { v->visit(this); }
};

// Type id ( ParamList ) Block
class FuncDecl : public TopDecl {
public:
    TypeNode*          return_type;
    std::string        name;
    std::vector<Param> params;
    Block*             body;
    FuncDecl(TypeNode* rt, std::string n, std::vector<Param> p, Block* b)
        : return_type(rt), name(std::move(n)), params(std::move(p)), body(b) {}
    ~FuncDecl() override {
        delete return_type;
        for (auto& p : params) { delete p.type; }
        delete body;
    }
    void accept(Visitor* v) override { v->visit(this); }
};

// template < typename id > FuncDecl
class TemplateFuncDecl : public TopDecl {
public:
    std::string template_param;  // nombre del typename, ej. "T"
    FuncDecl*   func;
    TemplateFuncDecl(std::string tp, FuncDecl* f)
        : template_param(std::move(tp)), func(f) {}
    ~TemplateFuncDecl() override { delete func; }
    void accept(Visitor* v) override { v->visit(this); }
};


// ═══════════════════════════════════════════════════════════════════════════
// PROGRAMA
// ═══════════════════════════════════════════════════════════════════════════

class Program {
public:
    std::vector<TopDecl*> decls;
    explicit Program(std::vector<TopDecl*> d) : decls(std::move(d)) {}
    ~Program() { for (auto d : decls) delete d; }
    void accept(Visitor* v) { v->visit(this); }
};
