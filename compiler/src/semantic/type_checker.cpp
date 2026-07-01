#include "type_checker.h"

// ─── Constructor ──────────────────────────────────────────────────────────────

TypeChecker::TypeChecker() {
    registerBuiltins();
}

void TypeChecker::registerBuiltins() {
    FuncInfo p;
    p.return_type = SemType{"void"};
    p.is_variadic = true;
    funcs_["print"]   = p;
    funcs_["println"] = p;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

void TypeChecker::semError(const std::string& msg, int line, int col) {
    throw SemanticError(msg, line, col);
}

bool TypeChecker::isValidBase(const std::string& base) const {
    if (base == "int" || base == "float" || base == "bool" ||
        base == "char" || base == "string" || base == "void")
        return true;
    return structs_.count(base) > 0;
}

SemType TypeChecker::resolveType(const TypeNode* node, int line, int col) {
    if (!node) return SemType{"void"};
    if (node->is_auto) return SemType{"auto"};
    if (!isValidBase(node->base))
        semError("unknown type '" + node->base + "'", line, col);
    return SemType::fromTypeNode(node);
}

bool TypeChecker::isLvalue(Expr* e) const {
    if (dynamic_cast<IdExpr*>(e))     return true;
    if (dynamic_cast<IndexExpr*>(e))  return true;
    if (dynamic_cast<MemberExpr*>(e)) return true;
    if (auto* u = dynamic_cast<UnaryExpr*>(e))
        return u->op == UnaryOp::Deref;
    return false;
}

bool TypeChecker::isArithmetic(const SemType& t) const {
    return !t.hasPointer() &&
           (t.base == "int" || t.base == "float" ||
            t.base == "bool" || t.base == "char");
}

bool TypeChecker::isCondition(const SemType& t) const {
    return !t.hasPointer() &&
           (t.base == "bool" || t.base == "int" ||
            t.base == "float" || t.base == "char");
}

SemType TypeChecker::visitExpr(Expr* e) {
    e->accept(this);
    return expr_type_;
}

bool TypeChecker::bodyHasReturn(Stmt* s) const {
    if (auto* r = dynamic_cast<ReturnStmt*>(s))
        return r->expr != nullptr;
    if (auto* b = dynamic_cast<Block*>(s)) {
        for (auto* st : b->stmts)
            if (bodyHasReturn(st)) return true;
        return false;
    }
    if (auto* i = dynamic_cast<IfStmt*>(s))
        return i->else_branch &&
               bodyHasReturn(i->then_branch) &&
               bodyHasReturn(i->else_branch);
    // while/for no garantizan ejecución del cuerpo
    return false;
}

// ─── Primera pasada ───────────────────────────────────────────────────────────

void TypeChecker::firstPass(Program* program) {
    // Sub-pasada 1: registrar nombres de structs para referencias cruzadas
    for (auto* decl : program->decls)
        if (auto* s = dynamic_cast<StructDecl*>(decl))
            structs_[s->name] = {};

    // Sub-pasada 2: llenar miembros de structs y firmas de funciones
    for (auto* decl : program->decls) {
        if (auto* s = dynamic_cast<StructDecl*>(decl)) {
            if (structs_.count(s->name) && !structs_[s->name].members.empty())
                semError("struct '" + s->name + "' already declared", s->line, s->col);
            StructInfo info;
            for (auto& m : s->members) {
                SemType mt = resolveType(m.type, s->line, s->col);
                if (mt.isVoid())
                    semError("member '" + m.name + "' cannot be void", s->line, s->col);
                if (mt.base == "auto")
                    semError("member '" + m.name + "' cannot be auto", s->line, s->col);
                if (info.find(m.name))
                    semError("duplicate member '" + m.name + "' in struct '" + s->name + "'", s->line, s->col);
                info.members.push_back({m.name, mt});
            }
            structs_[s->name] = info;

        } else if (auto* f = dynamic_cast<FuncDecl*>(decl)) {
            if (funcs_.count(f->name))
                semError("function '" + f->name + "' already declared", f->line, f->col);
            FuncInfo info;
            info.return_type = resolveType(f->return_type, f->line, f->col);
            for (auto& p : f->params) {
                SemType pt = resolveType(p.type, f->line, f->col);
                if (pt.isVoid())
                    semError("parameter '" + p.name + "' cannot be void", f->line, f->col);
                info.params.push_back({pt});
            }
            funcs_[f->name] = info;
        }
    }
}

// ─── Entrada pública ──────────────────────────────────────────────────────────

void TypeChecker::check(Program* program) {
    firstPass(program);
    program->accept(this);
}

void TypeChecker::visit(Program* node) {
    for (auto* decl : node->decls)
        decl->accept(this);
}

// ─── Declaraciones globales ───────────────────────────────────────────────────

void TypeChecker::visit(StructDecl*) {
    // Validado en primera pasada
}

void TypeChecker::visit(FuncDecl* node) {
    ret_type_     = resolveType(node->return_type, node->line, node->col);
    bool prev_loop = in_loop_;
    in_loop_      = false;

    vars_.enterScope();
    int int_params = 0, float_params = 0;  // bancos de la convención de llamada
    for (auto& p : node->params) {
        SemType pt = resolveType(p.type, node->line, node->col);
        if (pt.isVoid())
            semError("parameter '" + p.name + "' cannot be void", node->line, node->col);
        if (!vars_.declare(p.name, {pt}))
            semError("duplicate parameter '" + p.name + "'", node->line, node->col);
        (pt.isFloat() ? float_params : int_params)++;
    }
    if (int_params > 6 || float_params > 8)
        semError("function '" + node->name + "' has too many parameters (max 6 integer, 8 float)", node->line, node->col);

    if (!ret_type_.isVoid()) {
        if (!bodyHasReturn(node->body))
            semError("non-void function '" + node->name + "' must return a value", node->line, node->col);
    }

    node->body->accept(this);
    vars_.exitScope();
    in_loop_ = prev_loop;
}

// ─── Sentencias ───────────────────────────────────────────────────────────────

void TypeChecker::visit(Block* node) {
    vars_.enterScope();
    for (auto* s : node->stmts)
        s->accept(this);
    vars_.exitScope();
}

void TypeChecker::visit(VarDeclStmt* node) {
    SemType t;
    SemType init_type;
    bool has_init = node->init != nullptr;

    if (has_init) init_type = visitExpr(node->init);

    if (node->type->is_auto) {
        if (!has_init)
            semError("'auto' requires an initializer", node->line, node->col);
        t = init_type;
    } else {
        t = resolveType(node->type, node->line, node->col);
        if (t.isVoid())
            semError("variable '" + node->name + "' cannot be void", node->line, node->col);
    }

    for (auto* dim : node->dimensions) {
        SemType dt = visitExpr(dim);
        if (!dt.isIntegral())
            semError("array dimension must be int", node->line, node->col);
    }

    for (auto* elem : node->init_list) {
        SemType et = visitExpr(elem);
        if (!t.accepts(et))
            semError("incompatible type in initializer list of '" + node->name + "'", node->line, node->col);
    }

    if (has_init && !node->type->is_auto) {
        if (!t.accepts(init_type))
            semError("incompatible type in initializer of '" + node->name + "'", node->line, node->col);
    }

    // Un array estático decae a puntero: cada dimensión añade un nivel de
    // indirección al tipo de la variable. Así 'int arr[3]' se registra como
    // 'int*' (e 'int m[2][3]' como 'int**'), y arr[i] / m[i][j] tipan bien.
    // 't' se mantiene como tipo elemento para el chequeo del init_list de arriba.
    SemType var_t = t;
    for (size_t i = 0; i < node->dimensions.size(); ++i)
        var_t.mods.push_back(PtrMod::Pointer);

    if (!vars_.declare(node->name, {var_t}))
        semError("redeclaration of '" + node->name + "' in this scope", node->line, node->col);
}

void TypeChecker::visit(ExprStmt* node) {
    visitExpr(node->expr);
}

void TypeChecker::visit(IfStmt* node) {
    SemType ct = visitExpr(node->condition);
    if (!isCondition(ct))
        semError("if condition must be bool, int, float or char", node->line, node->col);
    node->then_branch->accept(this);
    if (node->else_branch) node->else_branch->accept(this);
}

void TypeChecker::visit(WhileStmt* node) {
    SemType ct = visitExpr(node->condition);
    if (!isCondition(ct))
        semError("while condition must be bool, int, float or char", node->line, node->col);
    bool prev = in_loop_;
    in_loop_ = true;
    node->body->accept(this);
    in_loop_ = prev;
}

void TypeChecker::visit(ForStmt* node) {
    vars_.enterScope();
    if (node->init.decl)       node->init.decl->accept(this);
    else if (node->init.expr)  visitExpr(node->init.expr);

    if (node->condition) {
        SemType ct = visitExpr(node->condition);
        if (!isCondition(ct))
            semError("for condition must be bool, int, float or char", node->line, node->col);
    }
    if (node->update) visitExpr(node->update);

    bool prev = in_loop_;
    in_loop_ = true;
    node->body->accept(this);
    in_loop_ = prev;
    vars_.exitScope();
}

void TypeChecker::visit(ReturnStmt* node) {
    if (ret_type_.isVoid()) {
        if (node->expr)
            semError("void function cannot return a value", node->line, node->col);
    } else {
        if (!node->expr)
            semError("non-void function must return a value", node->line, node->col);
        SemType et = visitExpr(node->expr);
        if (!ret_type_.accepts(et))
            semError("incompatible return type: expected " + ret_type_.toString(), node->line, node->col);
    }
}

void TypeChecker::visit(BreakStmt* node) {
    if (!in_loop_)
        semError("'break' outside of a loop", node->line, node->col);
}

void TypeChecker::visit(ContinueStmt* node) {
    if (!in_loop_)
        semError("'continue' outside of a loop", node->line, node->col);
}

void TypeChecker::visit(DeleteStmt* node) {
    SemType t = visitExpr(node->expr);
    if (!t.hasPointer())
        semError("'delete' requires a pointer", node->line, node->col);
}

// ─── Expresiones ─────────────────────────────────────────────────────────────

void TypeChecker::visit(IntLitExpr*)    { expr_type_ = SemType{"int"};    }
void TypeChecker::visit(FloatLitExpr*)  { expr_type_ = SemType{"float"};  }
void TypeChecker::visit(BoolLitExpr*)   { expr_type_ = SemType{"bool"};   }
void TypeChecker::visit(CharLitExpr*)   { expr_type_ = SemType{"char"};   }
void TypeChecker::visit(StringLitExpr*) { expr_type_ = SemType{"string"}; }

void TypeChecker::visit(IdExpr* node) {
    VarInfo* v = vars_.lookup(node->name);
    if (v) { expr_type_ = v->type; return; }
    if (funcs_.count(node->name)) { expr_type_ = SemType{"void"}; return; }
    semError("use of undeclared variable '" + node->name + "'", node->line, node->col);
}

void TypeChecker::visit(BinaryExpr* node) {
    SemType lt = visitExpr(node->left);
    SemType rt = visitExpr(node->right);

    switch (node->op) {
        case BinaryOp::Add: case BinaryOp::Sub:
        case BinaryOp::Mul: case BinaryOp::Div:
            if (!isArithmetic(lt) || !isArithmetic(rt))
                semError("arithmetic operands must be numeric", node->line, node->col);
            expr_type_ = SemType::promote(lt, rt);
            break;
        case BinaryOp::Mod:
            if (!lt.isIntegral() || !rt.isIntegral())
                semError("'%' requires int operands", node->line, node->col);
            expr_type_ = SemType{"int"};
            break;
        case BinaryOp::Eq: case BinaryOp::Neq:
        case BinaryOp::Lt: case BinaryOp::Gt:
        case BinaryOp::Leq: case BinaryOp::Geq:
            if (!isArithmetic(lt) || !isArithmetic(rt))
                semError("incompatible operands in comparison", node->line, node->col);
            expr_type_ = SemType{"bool"};
            break;
        case BinaryOp::And: case BinaryOp::Or:
            if (!isCondition(lt) || !isCondition(rt))
                semError("logical operands must be bool or numeric", node->line, node->col);
            expr_type_ = SemType{"bool"};
            break;
    }
}

void TypeChecker::visit(UnaryExpr* node) {
    SemType t = visitExpr(node->expr);

    switch (node->op) {
        case UnaryOp::Neg:
            if (!isArithmetic(t))
                semError("'-' requires a numeric operand", node->line, node->col);
            expr_type_ = t;
            break;
        case UnaryOp::Not:
            if (!isCondition(t))
                semError("'!' requires a bool or numeric operand", node->line, node->col);
            expr_type_ = SemType{"bool"};
            break;
        case UnaryOp::Deref:
            if (!t.hasPointer())
                semError("'*' requires a pointer", node->line, node->col);
            expr_type_ = t.deref();
            break;
        case UnaryOp::AddrOf:
            if (!isLvalue(node->expr))
                semError("'&' requires an lvalue", node->line, node->col);
            { SemType pt = t; pt.mods.push_back(PtrMod::Pointer); expr_type_ = pt; }
            break;
        case UnaryOp::PreInc: case UnaryOp::PreDec:
            if (!isLvalue(node->expr))
                semError("prefix ++/-- requires an lvalue", node->line, node->col);
            if (!isArithmetic(t) && !t.hasPointer())
                semError("++/-- requires a numeric or pointer type", node->line, node->col);
            expr_type_ = t;
            break;
    }
}

void TypeChecker::visit(AssignExpr* node) {
    if (!isLvalue(node->left))
        semError("left-hand side of assignment must be an lvalue", node->line, node->col);

    SemType lt = visitExpr(node->left);
    SemType rt = visitExpr(node->right);

    if (!lt.accepts(rt))
        semError("incompatible type in assignment", node->line, node->col);
    expr_type_ = lt;
}

void TypeChecker::visit(NewArrayExpr* node) {
    SemType t = resolveType(node->type, node->line, node->col);
    if (t.isVoid())
        semError("new[] type cannot be void", node->line, node->col);
    SemType sz = visitExpr(node->size);
    if (!sz.isIntegral())
        semError("new[] size must be int", node->line, node->col);
    SemType pt = t;
    pt.mods.push_back(PtrMod::Pointer);
    expr_type_ = pt;
}

void TypeChecker::visit(NewObjectExpr* node) {
    if (!structs_.count(node->type->base))
        semError("type '" + node->type->base + "' is not a declared struct", node->line, node->col);
    SemType t = resolveType(node->type, node->line, node->col);
    SemType pt = t;
    pt.mods.push_back(PtrMod::Pointer);
    expr_type_ = pt;
}

void TypeChecker::visit(IndexExpr* node) {
    SemType bt = visitExpr(node->base);
    if (!bt.hasPointer() && bt.base != "string")
        semError("subscript requires an array, pointer or string", node->line, node->col);
    SemType it = visitExpr(node->index);
    if (!it.isIntegral())
        semError("subscript index must be int", node->line, node->col);
    expr_type_ = (bt.base == "string") ? SemType{"char"} : bt.deref();
}

void TypeChecker::visit(CallExpr* node) {
    auto* id = dynamic_cast<IdExpr*>(node->callee);

    // Solo se puede llamar a una función de usuario o built-in por nombre, y
    // siempre que no esté sombreada por una variable del mismo nombre.
    bool isNamedFunc = id && funcs_.count(id->name) && !vars_.lookup(id->name);
    if (!isNamedFunc) {
        if (id && !vars_.lookup(id->name))
            semError("call to undeclared function '" + id->name + "'", node->line, node->col);
        semError("call target is not callable", node->line, node->col);
    }

    const FuncInfo& fi = funcs_[id->name];

    if (fi.is_variadic) {
        if (node->args.empty())
            semError("print/println requires at least one argument", node->line, node->col);
        for (auto* a : node->args) {
            SemType at = visitExpr(a);
            if (at.hasPointer() || structs_.count(at.base) || at.base == "void")
                semError("print/println argument must be a basic type", node->line, node->col);
        }
        expr_type_ = SemType{"void"};
        return;
    }

    if (node->args.size() != fi.params.size())
        semError("wrong number of arguments in call to '" + id->name + "'", node->line, node->col);

    for (size_t i = 0; i < node->args.size(); ++i) {
        SemType at = visitExpr(node->args[i]);
        if (!fi.params[i].type.accepts(at))
            semError("argument " + std::to_string(i+1) + " incompatible in call to '" + id->name + "'", node->line, node->col);
    }

    expr_type_ = fi.return_type;
}

void TypeChecker::visit(MemberExpr* node) {
    SemType bt = visitExpr(node->base);

    std::string sname;
    if (node->is_arrow) {
        if (!bt.hasPointer())
            semError("'->' requires a pointer to struct", node->line, node->col);
        sname = bt.deref().base;
    } else {
        if (bt.hasPointer())
            semError("'.' requires a struct, not a pointer (use '->')", node->line, node->col);
        sname = bt.base;
    }

    auto sit = structs_.find(sname);
    if (sit == structs_.end())
        semError("'" + sname + "' is not a struct", node->line, node->col);
    const MemberInfo* m = sit->second.find(node->member);
    if (!m)
        semError("struct '" + sname + "' has no member '" + node->member + "'", node->line, node->col);
    expr_type_ = m->type;
}

void TypeChecker::visit(PostfixExpr* node) {
    if (!isLvalue(node->base))
        semError("postfix ++/-- requires an lvalue", node->line, node->col);
    SemType t = visitExpr(node->base);
    if (!isArithmetic(t) && !t.hasPointer())
        semError("postfix ++/-- requires a numeric or pointer type", node->line, node->col);
    expr_type_ = t;
}
