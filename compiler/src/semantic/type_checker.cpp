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
    if (in_template_ && base == template_param_) return true;
    return structs_.count(base) > 0;
}

SemType TypeChecker::resolveType(const TypeNode* node, int line, int col) {
    if (!node) return SemType{"void"};
    if (node->is_auto) return SemType{"auto"};
    if (!isValidBase(node->base))
        semError("Tipo desconocido: '" + node->base + "'", line, col);
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

bool TypeChecker::isTemplateType(const SemType& t) const {
    return in_template_ && !template_param_.empty() && t.base == template_param_;
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

    // Sub-pasada 2: llenar miembros de structs, funciones y variables globales
    for (auto* decl : program->decls) {
        if (auto* s = dynamic_cast<StructDecl*>(decl)) {
            if (structs_.count(s->name) && !structs_[s->name].members.empty())
                semError("Struct '" + s->name + "' ya declarado", s->line, s->col);
            StructInfo info;
            for (auto& m : s->members) {
                SemType mt = resolveType(m.type, s->line, s->col);
                if (mt.isVoid())
                    semError("Miembro '" + m.name + "' no puede ser void", s->line, s->col);
                if (mt.base == "auto")
                    semError("Miembro '" + m.name + "' no puede ser auto", s->line, s->col);
                if (info.find(m.name))
                    semError("Miembro duplicado '" + m.name + "' en struct '" + s->name + "'", s->line, s->col);
                info.members.push_back({m.name, mt});
            }
            structs_[s->name] = info;

        } else if (auto* f = dynamic_cast<FuncDecl*>(decl)) {
            if (funcs_.count(f->name))
                semError("Función '" + f->name + "' ya declarada", f->line, f->col);
            FuncInfo info;
            info.return_type = resolveType(f->return_type, f->line, f->col);
            for (auto& p : f->params) {
                SemType pt = resolveType(p.type, f->line, f->col);
                if (pt.isVoid())
                    semError("Parámetro '" + p.name + "' no puede ser void", f->line, f->col);
                info.params.push_back({pt, p.is_ref, p.default_val != nullptr});
            }
            funcs_[f->name] = info;

        } else if (auto* tf = dynamic_cast<TemplateFuncDecl*>(decl)) {
            auto* f = tf->func;
            if (funcs_.count(f->name))
                semError("Función '" + f->name + "' ya declarada", f->line, f->col);
            FuncInfo info;
            info.template_param = tf->template_param;
            info.return_type = f->return_type
                ? SemType{f->return_type->base}
                : SemType{"void"};
            for (auto& p : f->params) {
                SemType pt{p.type ? p.type->base : "void"};
                info.params.push_back({pt, p.is_ref, p.default_val != nullptr});
            }
            funcs_[f->name] = info;

        } else if (auto* g = dynamic_cast<GlobalVarDecl*>(decl)) {
            SemType gt = resolveType(g->type, g->line, g->col);
            if (gt.isVoid())
                semError("Variable '" + g->name + "' no puede ser void", g->line, g->col);
            if (!vars_.declare(g->name, {gt, g->is_const}))
                semError("Variable global '" + g->name + "' ya declarada", g->line, g->col);
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

void TypeChecker::visit(GlobalVarDecl* node) {
    SemType t = resolveType(node->type, node->line, node->col);
    if (node->is_const && !node->init)
        semError("const '" + node->name + "' requiere inicializador", node->line, node->col);
    if (node->init) {
        SemType it = visitExpr(node->init);
        if (!t.accepts(it) && !isTemplateType(t))
            semError("Tipo incompatible en inicializador de '" + node->name + "'", node->line, node->col);
    }
}

void TypeChecker::visit(StructDecl* node) {
    // Validado en primera pasada
}

void TypeChecker::visit(FuncDecl* node) {
    ret_type_     = resolveType(node->return_type, node->line, node->col);
    bool prev_loop = in_loop_;
    in_loop_      = false;

    vars_.enterScope();
    for (auto& p : node->params) {
        SemType pt = resolveType(p.type, node->line, node->col);
        if (pt.isVoid())
            semError("Parámetro '" + p.name + "' no puede ser void", node->line, node->col);
        if (!vars_.declare(p.name, {pt, p.is_const}))
            semError("Parámetro duplicado '" + p.name + "'", node->line, node->col);
        if (p.default_val) {
            SemType dv = visitExpr(p.default_val);
            if (!pt.accepts(dv) && !isTemplateType(pt) && !isTemplateType(dv))
                semError("Tipo incompatible en valor default de '" + p.name + "'", node->line, node->col);
        }
    }

    if (!ret_type_.isVoid() && !isTemplateType(ret_type_)) {
        if (!bodyHasReturn(node->body))
            semError("Función '" + node->name + "' debe tener al menos un return con valor", node->line, node->col);
    }

    node->body->accept(this);
    vars_.exitScope();
    in_loop_ = prev_loop;
}

void TypeChecker::visit(TemplateFuncDecl* node) {
    in_template_    = true;
    template_param_ = node->template_param;
    visit(node->func);
    in_template_    = false;
    template_param_ = "";
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
            semError("'auto' requiere inicializador", node->line, node->col);
        t = init_type;
    } else {
        t = resolveType(node->type, node->line, node->col);
        if (t.isVoid())
            semError("Variable '" + node->name + "' no puede ser void", node->line, node->col);
    }

    if (node->is_const && !has_init && node->dimensions.empty())
        semError("const '" + node->name + "' requiere inicializador", node->line, node->col);

    for (auto* dim : node->dimensions) {
        SemType dt = visitExpr(dim);
        if (!dt.isIntegral())
            semError("Dimensión de array debe ser int", node->line, node->col);
    }

    for (auto* elem : node->init_list) {
        SemType et = visitExpr(elem);
        if (!t.accepts(et) && !isTemplateType(t))
            semError("Tipo incompatible en init list de '" + node->name + "'", node->line, node->col);
    }

    if (has_init && !node->type->is_auto) {
        if (!t.accepts(init_type) && !isTemplateType(t) && !isTemplateType(init_type))
            semError("Tipo incompatible en inicializador de '" + node->name + "'", node->line, node->col);
    }

    if (!vars_.declare(node->name, {t, node->is_const}))
        semError("Variable '" + node->name + "' ya declarada en este scope", node->line, node->col);
}

void TypeChecker::visit(ExprStmt* node) {
    visitExpr(node->expr);
}

void TypeChecker::visit(IfStmt* node) {
    SemType ct = visitExpr(node->condition);
    if (!isCondition(ct))
        semError("Condición de if debe ser bool, int, float o char", node->line, node->col);
    node->then_branch->accept(this);
    if (node->else_branch) node->else_branch->accept(this);
}

void TypeChecker::visit(WhileStmt* node) {
    SemType ct = visitExpr(node->condition);
    if (!isCondition(ct))
        semError("Condición de while debe ser bool, int, float o char", node->line, node->col);
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
            semError("Condición de for debe ser bool, int, float o char", node->line, node->col);
    }
    if (node->update) visitExpr(node->update);

    bool prev = in_loop_;
    in_loop_ = true;
    node->body->accept(this);
    in_loop_ = prev;
    vars_.exitScope();
}

void TypeChecker::visit(ForRangeStmt* node) {
    vars_.enterScope();
    SemType iter_t = visitExpr(node->iterable);

    SemType elem_t;
    if (iter_t.hasPointer())         elem_t = iter_t.deref();
    else if (iter_t.base == "string") elem_t = SemType{"char"};
    else                              elem_t = iter_t; // array estático: mismo tipo base

    SemType var_t;
    if (node->type->is_auto) {
        var_t = elem_t;
    } else {
        var_t = resolveType(node->type, node->line, node->col);
        if (!var_t.accepts(elem_t) && !isTemplateType(var_t))
            semError("Tipo de variable de rango incompatible con iterable", node->line, node->col);
    }

    if (!vars_.declare(node->name, {var_t, node->is_const}))
        semError("Variable '" + node->name + "' ya declarada", node->line, node->col);

    bool prev = in_loop_;
    in_loop_ = true;
    node->body->accept(this);
    in_loop_ = prev;
    vars_.exitScope();
}

void TypeChecker::visit(ReturnStmt* node) {
    if (ret_type_.isVoid()) {
        if (node->expr)
            semError("Función void no puede retornar un valor", node->line, node->col);
    } else {
        if (!node->expr)
            semError("Función no-void debe retornar un valor", node->line, node->col);
        SemType et = visitExpr(node->expr);
        if (!isTemplateType(ret_type_) && !isTemplateType(et) && !ret_type_.accepts(et))
            semError("Tipo de retorno incompatible: se esperaba " + ret_type_.toString(), node->line, node->col);
    }
}

void TypeChecker::visit(BreakStmt* node) {
    if (!in_loop_)
        semError("'break' fuera de un bucle", node->line, node->col);
}

void TypeChecker::visit(ContinueStmt* node) {
    if (!in_loop_)
        semError("'continue' fuera de un bucle", node->line, node->col);
}

void TypeChecker::visit(DeleteStmt* node) {
    SemType t = visitExpr(node->expr);
    if (!t.hasPointer())
        semError("'delete' requiere un puntero", node->line, node->col);
}

// ─── Expresiones ─────────────────────────────────────────────────────────────

void TypeChecker::visit(IntLitExpr* node)    { expr_type_ = SemType{"int"};    }
void TypeChecker::visit(FloatLitExpr* node)  { expr_type_ = SemType{"float"};  }
void TypeChecker::visit(BoolLitExpr* node)   { expr_type_ = SemType{"bool"};   }
void TypeChecker::visit(CharLitExpr* node)   { expr_type_ = SemType{"char"};   }
void TypeChecker::visit(StringLitExpr* node) { expr_type_ = SemType{"string"}; }

void TypeChecker::visit(IdExpr* node) {
    VarInfo* v = vars_.lookup(node->name);
    if (v) { expr_type_ = v->type; return; }
    if (funcs_.count(node->name)) { expr_type_ = SemType{"void"}; return; }
    semError("Variable '" + node->name + "' no declarada", node->line, node->col);
}

void TypeChecker::visit(BinaryExpr* node) {
    SemType lt = visitExpr(node->left);
    SemType rt = visitExpr(node->right);

    // Saltar verificación si hay tipos template
    if (isTemplateType(lt) || isTemplateType(rt)) {
        switch (node->op) {
            case BinaryOp::Eq:  case BinaryOp::Neq:
            case BinaryOp::Lt:  case BinaryOp::Gt:
            case BinaryOp::Leq: case BinaryOp::Geq:
            case BinaryOp::And: case BinaryOp::Or:
                expr_type_ = SemType{"bool"};
                break;
            default:
                expr_type_ = isTemplateType(lt) ? rt : lt;
        }
        return;
    }

    switch (node->op) {
        case BinaryOp::Add: case BinaryOp::Sub:
        case BinaryOp::Mul: case BinaryOp::Div:
            if (!isArithmetic(lt) || !isArithmetic(rt))
                semError("Operandos aritméticos deben ser numéricos", node->line, node->col);
            expr_type_ = SemType::promote(lt, rt);
            break;
        case BinaryOp::Mod:
            if (!lt.isIntegral() || !rt.isIntegral())
                semError("'%' requiere operandos int", node->line, node->col);
            expr_type_ = SemType{"int"};
            break;
        case BinaryOp::Eq: case BinaryOp::Neq:
        case BinaryOp::Lt: case BinaryOp::Gt:
        case BinaryOp::Leq: case BinaryOp::Geq:
            if (!isArithmetic(lt) || !isArithmetic(rt))
                semError("Operandos de comparación incompatibles", node->line, node->col);
            expr_type_ = SemType{"bool"};
            break;
        case BinaryOp::And: case BinaryOp::Or:
            if (!isCondition(lt) || !isCondition(rt))
                semError("Operandos lógicos deben ser bool o numéricos", node->line, node->col);
            expr_type_ = SemType{"bool"};
            break;
    }
}

void TypeChecker::visit(UnaryExpr* node) {
    SemType t = visitExpr(node->expr);
    if (isTemplateType(t)) { expr_type_ = t; return; }

    switch (node->op) {
        case UnaryOp::Neg:
            if (!isArithmetic(t))
                semError("'-' requiere operando numérico", node->line, node->col);
            expr_type_ = t;
            break;
        case UnaryOp::Not:
            if (!isCondition(t))
                semError("'!' requiere operando bool o numérico", node->line, node->col);
            expr_type_ = SemType{"bool"};
            break;
        case UnaryOp::BitNot:
            if (!t.isIntegral())
                semError("'~' requiere operando int", node->line, node->col);
            expr_type_ = t;
            break;
        case UnaryOp::Deref:
            if (!t.hasPointer())
                semError("'*' requiere puntero", node->line, node->col);
            expr_type_ = t.deref();
            break;
        case UnaryOp::AddrOf:
            if (!isLvalue(node->expr))
                semError("'&' requiere lvalue", node->line, node->col);
            { SemType pt = t; pt.mods.push_back(PtrMod::Pointer); expr_type_ = pt; }
            break;
        case UnaryOp::PreInc: case UnaryOp::PreDec:
            if (!isLvalue(node->expr))
                semError("++/-- prefijo requiere lvalue", node->line, node->col);
            if (!isArithmetic(t) && !t.hasPointer())
                semError("++/-- requiere tipo numérico o puntero", node->line, node->col);
            expr_type_ = t;
            break;
    }
}

void TypeChecker::visit(AssignExpr* node) {
    if (!isLvalue(node->left))
        semError("El lado izquierdo de la asignación debe ser un lvalue", node->line, node->col);

    SemType lt = visitExpr(node->left);
    SemType rt = visitExpr(node->right);

    if (auto* id = dynamic_cast<IdExpr*>(node->left)) {
        VarInfo* v = vars_.lookup(id->name);
        if (v && v->is_const)
            semError("No se puede asignar a const '" + id->name + "'", node->line, node->col);
    }

    if (isTemplateType(lt) || isTemplateType(rt)) { expr_type_ = lt; return; }

    switch (node->op) {
        case AssignOp::Assign:
            if (!lt.accepts(rt))
                semError("Tipo incompatible en asignación", node->line, node->col);
            break;
        case AssignOp::PlusAssign: case AssignOp::MinusAssign:
        case AssignOp::MulAssign:  case AssignOp::DivAssign:
            if (!isArithmetic(lt))
                semError("+=/-=/*=/= requiere lvalue numérico", node->line, node->col);
            break;
        case AssignOp::ModAssign:
            if (!lt.isIntegral())
                semError("'%=' requiere lvalue int", node->line, node->col);
            break;
        case AssignOp::AndAssign: case AssignOp::OrAssign:
            if (!lt.isIntegral())
                semError("'&='/'|=' requiere lvalue int", node->line, node->col);
            break;
    }
    expr_type_ = lt;
}

void TypeChecker::visit(CastExpr* node) {
    SemType target = resolveType(node->type, node->line, node->col);
    SemType src    = visitExpr(node->expr);
    auto castable  = [](const SemType& t) {
        return !t.hasPointer() &&
               (t.base=="int"||t.base=="float"||t.base=="char"||t.base=="bool");
    };
    if (!castable(target) || (!castable(src) && !isTemplateType(src)))
        semError("static_cast solo válido entre int, float, char, bool", node->line, node->col);
    expr_type_ = target;
}

void TypeChecker::visit(NewArrayExpr* node) {
    SemType t = resolveType(node->type, node->line, node->col);
    if (t.isVoid())
        semError("new[] no puede ser void", node->line, node->col);
    SemType sz = visitExpr(node->size);
    if (!sz.isIntegral())
        semError("Tamaño de new[] debe ser int", node->line, node->col);
    SemType pt = t;
    pt.mods.push_back(PtrMod::Pointer);
    expr_type_ = pt;
}

void TypeChecker::visit(NewObjectExpr* node) {
    if (!structs_.count(node->type->base))
        semError("Tipo '" + node->type->base + "' no es un struct declarado", node->line, node->col);
    SemType t = resolveType(node->type, node->line, node->col);
    SemType pt = t;
    pt.mods.push_back(PtrMod::Pointer);
    expr_type_ = pt;
}

void TypeChecker::visit(IndexExpr* node) {
    SemType bt = visitExpr(node->base);
    if (!bt.hasPointer() && bt.base != "string")
        semError("Indexado requiere array, puntero o string", node->line, node->col);
    SemType it = visitExpr(node->index);
    if (!it.isIntegral())
        semError("Índice debe ser int", node->line, node->col);
    expr_type_ = (bt.base == "string") ? SemType{"char"} : bt.deref();
}

void TypeChecker::visit(CallExpr* node) {
    auto* id = dynamic_cast<IdExpr*>(node->callee);
    if (!id) {
        visitExpr(node->callee);
        for (auto* a : node->args) visitExpr(a);
        expr_type_ = SemType{"void"};
        return;
    }

    auto it = funcs_.find(id->name);
    if (it == funcs_.end())
        semError("Función '" + id->name + "' no declarada", node->line, node->col);

    const FuncInfo& fi = it->second;

    if (fi.is_variadic) {
        if (node->args.empty())
            semError("print/println requiere al menos un argumento", node->line, node->col);
        for (auto* a : node->args) {
            SemType at = visitExpr(a);
            if (at.hasPointer() || structs_.count(at.base) || at.base == "void")
                semError("Argumento de print/println debe ser tipo básico", node->line, node->col);
        }
        expr_type_ = SemType{"void"};
        return;
    }

    size_t required = 0;
    for (auto& p : fi.params) if (!p.has_default) required++;
    if (node->args.size() < required || node->args.size() > fi.params.size())
        semError("Número de argumentos incorrecto en llamada a '" + id->name + "'", node->line, node->col);

    // Para funciones template: inferir T de los argumentos
    SemType resolved_T;
    bool    T_resolved = false;

    for (size_t i = 0; i < node->args.size(); ++i) {
        SemType at = visitExpr(node->args[i]);
        const SemType& pt = fi.params[i].type;
        bool pt_is_T = !fi.template_param.empty() && pt.base == fi.template_param;

        if (pt_is_T) {
            if (!T_resolved) {
                resolved_T = at;
                T_resolved = true;
            } else if (resolved_T != at) {
                if (isArithmetic(at) && isArithmetic(resolved_T))
                    resolved_T = SemType::promote(resolved_T, at);
                else
                    semError("Tipos inconsistentes en argumentos template de '" + id->name + "'", node->line, node->col);
            }
        } else if (!isTemplateType(pt) && !isTemplateType(at) && !pt.accepts(at)) {
            semError("Argumento " + std::to_string(i+1) + " incompatible en llamada a '" + id->name + "'", node->line, node->col);
        }
    }

    // Si el tipo de retorno es T, sustituir con el tipo concreto inferido
    if (!fi.template_param.empty() && fi.return_type.base == fi.template_param && T_resolved)
        expr_type_ = resolved_T;
    else
        expr_type_ = fi.return_type;
}

void TypeChecker::visit(MemberExpr* node) {
    SemType bt = visitExpr(node->base);
    if (isTemplateType(bt)) { expr_type_ = bt; return; }

    std::string sname;
    if (node->is_arrow) {
        if (!bt.hasPointer())
            semError("'->' requiere puntero a struct", node->line, node->col);
        sname = bt.deref().base;
    } else {
        if (bt.hasPointer())
            semError("'.' requiere struct, no puntero (use '->')", node->line, node->col);
        sname = bt.base;
    }

    auto sit = structs_.find(sname);
    if (sit == structs_.end())
        semError("'" + sname + "' no es un struct", node->line, node->col);
    const MemberInfo* m = sit->second.find(node->member);
    if (!m)
        semError("Struct '" + sname + "' no tiene miembro '" + node->member + "'", node->line, node->col);
    expr_type_ = m->type;
}

void TypeChecker::visit(PostfixExpr* node) {
    if (!isLvalue(node->base))
        semError("Postfix ++/-- requiere lvalue", node->line, node->col);
    SemType t = visitExpr(node->base);
    if (!isArithmetic(t) && !t.hasPointer())
        semError("Postfix ++/-- requiere tipo numérico o puntero", node->line, node->col);
    expr_type_ = t;
}

void TypeChecker::visit(LambdaExpr* node) {
    for (auto& c : node->captures)
        if (!c.name.empty() && !vars_.lookup(c.name))
            semError("Variable capturada '" + c.name + "' no existe en el scope", node->line, node->col);

    SemType prev_ret  = ret_type_;
    bool    prev_loop = in_loop_;
    in_loop_ = false;

    ret_type_ = node->return_type
        ? resolveType(node->return_type, node->line, node->col)
        : SemType{"void"};

    vars_.enterScope();
    for (auto& p : node->params) {
        SemType pt = resolveType(p.type, node->line, node->col);
        vars_.declare(p.name, {pt, p.is_const});
    }
    node->body->accept(this);
    vars_.exitScope();

    ret_type_ = prev_ret;
    in_loop_  = prev_loop;
    expr_type_ = SemType{"void"};
}
