#include "ast_printer.h"
#include "ast.h"

// ─── Helpers ──────────────────────────────────────────────────────────────────

void ASTPrinter::indent() {
    for (int i = 0; i < depth; i++) out << "  ";
}

std::string ASTPrinter::typeStr(TypeNode* t) {
    if (!t) return "(null)";
    if (t->is_auto) return "auto";
    std::string s;
    if (t->is_const) s += "const ";
    s += t->base;
    if (t->template_arg) s += "<" + typeStr(t->template_arg) + ">";
    for (auto mod : t->mods)
        s += (mod == PtrMod::Pointer ? "*" : "&");
    return s;
}

const char* ASTPrinter::binaryOpStr(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add: return "+";
        case BinaryOp::Sub: return "-";
        case BinaryOp::Mul: return "*";
        case BinaryOp::Div: return "/";
        case BinaryOp::Mod: return "%";
        case BinaryOp::Eq:  return "==";
        case BinaryOp::Neq: return "!=";
        case BinaryOp::Lt:  return "<";
        case BinaryOp::Gt:  return ">";
        case BinaryOp::Leq: return "<=";
        case BinaryOp::Geq: return ">=";
        case BinaryOp::And: return "&&";
        case BinaryOp::Or:  return "||";
    }
    return "?";
}

const char* ASTPrinter::assignOpStr(AssignOp op) {
    switch (op) {
        case AssignOp::Assign:      return "=";
        case AssignOp::PlusAssign:  return "+=";
        case AssignOp::MinusAssign: return "-=";
        case AssignOp::MulAssign:   return "*=";
        case AssignOp::DivAssign:   return "/=";
        case AssignOp::ModAssign:   return "%=";
        case AssignOp::AndAssign:   return "&=";
        case AssignOp::OrAssign:    return "|=";
    }
    return "?";
}

const char* ASTPrinter::unaryOpStr(UnaryOp op) {
    switch (op) {
        case UnaryOp::Neg:    return "-";
        case UnaryOp::Not:    return "!";
        case UnaryOp::BitNot: return "~";
        case UnaryOp::Deref:  return "*";
        case UnaryOp::AddrOf: return "&";
        case UnaryOp::PreInc: return "++";
        case UnaryOp::PreDec: return "--";
    }
    return "?";
}

// ─── Program ──────────────────────────────────────────────────────────────────

void ASTPrinter::visit(Program* node) {
    out << "Program\n";
    depth++;
    for (auto d : node->decls) d->accept(this);
    depth--;
}

// ─── Literals ─────────────────────────────────────────────────────────────────

void ASTPrinter::visit(IntLitExpr* node) {
    indent(); out << "IntLit(" << node->value << ")\n";
}

void ASTPrinter::visit(FloatLitExpr* node) {
    indent(); out << "FloatLit(" << node->value << ")\n";
}

void ASTPrinter::visit(BoolLitExpr* node) {
    indent(); out << "BoolLit(" << (node->value ? "true" : "false") << ")\n";
}

void ASTPrinter::visit(CharLitExpr* node) {
    indent(); out << "CharLit('" << node->value << "')\n";
}

void ASTPrinter::visit(StringLitExpr* node) {
    indent(); out << "StringLit(\"" << node->value << "\")\n";
}

void ASTPrinter::visit(IdExpr* node) {
    indent(); out << "Id(" << node->name << ")\n";
}

// ─── Operators ────────────────────────────────────────────────────────────────

void ASTPrinter::visit(BinaryExpr* node) {
    indent(); out << "BinaryExpr(" << binaryOpStr(node->op) << ")\n";
    depth++;
    node->left->accept(this);
    node->right->accept(this);
    depth--;
}

void ASTPrinter::visit(UnaryExpr* node) {
    indent(); out << "UnaryExpr(" << unaryOpStr(node->op) << ")\n";
    depth++;
    node->expr->accept(this);
    depth--;
}

void ASTPrinter::visit(AssignExpr* node) {
    indent(); out << "AssignExpr(" << assignOpStr(node->op) << ")\n";
    depth++;
    node->left->accept(this);
    node->right->accept(this);
    depth--;
}

void ASTPrinter::visit(CastExpr* node) {
    indent(); out << "CastExpr -> " << typeStr(node->type) << "\n";
    depth++;
    node->expr->accept(this);
    depth--;
}

// ─── Memory ───────────────────────────────────────────────────────────────────

void ASTPrinter::visit(NewArrayExpr* node) {
    indent(); out << "NewArray(" << typeStr(node->type) << ")\n";
    depth++;
    node->size->accept(this);
    depth--;
}

void ASTPrinter::visit(NewObjectExpr* node) {
    indent(); out << "NewObject(" << typeStr(node->type) << ")\n";
    depth++;
    for (auto a : node->args) a->accept(this);
    depth--;
}

// ─── Postfix ──────────────────────────────────────────────────────────────────

void ASTPrinter::visit(IndexExpr* node) {
    indent(); out << "IndexExpr\n";
    depth++;
    node->base->accept(this);
    node->index->accept(this);
    depth--;
}

void ASTPrinter::visit(CallExpr* node) {
    indent(); out << "CallExpr\n";
    depth++;
    node->callee->accept(this);
    for (auto a : node->args) a->accept(this);
    depth--;
}

void ASTPrinter::visit(MemberExpr* node) {
    indent(); out << "MemberExpr(" << (node->is_arrow ? "->" : ".") << node->member << ")\n";
    depth++;
    node->base->accept(this);
    depth--;
}

void ASTPrinter::visit(PostfixExpr* node) {
    indent(); out << "PostfixExpr(" << (node->is_inc ? "++" : "--") << ")\n";
    depth++;
    node->base->accept(this);
    depth--;
}

void ASTPrinter::visit(LambdaExpr* node) {
    indent(); out << "LambdaExpr";
    if (node->return_type) out << " -> " << typeStr(node->return_type);
    out << "\n";
    depth++;
    if (!node->captures.empty()) {
        indent(); out << "captures: [";
        for (size_t i = 0; i < node->captures.size(); i++) {
            if (i) out << ", ";
            auto& c = node->captures[i];
            if (c.name.empty()) out << (c.is_ref ? "&" : "=");
            else { if (c.is_ref) out << "&"; out << c.name; }
        }
        out << "]\n";
    }
    for (auto& p : node->params) {
        indent();
        if (p.is_const) out << "const ";
        out << "param '" << p.name << "' : " << typeStr(p.type);
        if (p.is_ref) out << "&";
        out << "\n";
        if (p.default_val) {
            depth++;
            p.default_val->accept(this);
            depth--;
        }
    }
    node->body->accept(this);
    depth--;
}

// ─── Statements ───────────────────────────────────────────────────────────────

void ASTPrinter::visit(Block* node) {
    indent(); out << "Block\n";
    depth++;
    for (auto s : node->stmts) s->accept(this);
    depth--;
}

void ASTPrinter::visit(VarDeclStmt* node) {
    indent();
    if (node->is_const) out << "const ";
    out << "VarDecl '" << node->name << "' : " << typeStr(node->type);
    if (!node->dimensions.empty()) out << "[]";
    out << "\n";
    depth++;
    if (!node->dimensions.empty()) {
        indent(); out << "dims:\n";
        depth++;
        for (auto d : node->dimensions) d->accept(this);
        depth--;
    }
    if (node->init) {
        indent(); out << "init:\n";
        depth++;
        node->init->accept(this);
        depth--;
    }
    if (!node->init_list.empty()) {
        indent(); out << "init_list:\n";
        depth++;
        for (auto e : node->init_list) e->accept(this);
        depth--;
    }
    depth--;
}

void ASTPrinter::visit(ExprStmt* node) {
    indent(); out << "ExprStmt\n";
    depth++;
    node->expr->accept(this);
    depth--;
}

void ASTPrinter::visit(IfStmt* node) {
    indent(); out << "IfStmt\n";
    depth++;
    indent(); out << "cond:\n";
    depth++; node->condition->accept(this); depth--;
    indent(); out << "then:\n";
    depth++; node->then_branch->accept(this); depth--;
    if (node->else_branch) {
        indent(); out << "else:\n";
        depth++; node->else_branch->accept(this); depth--;
    }
    depth--;
}

void ASTPrinter::visit(WhileStmt* node) {
    indent(); out << "WhileStmt\n";
    depth++;
    indent(); out << "cond:\n";
    depth++; node->condition->accept(this); depth--;
    indent(); out << "body:\n";
    depth++; node->body->accept(this); depth--;
    depth--;
}

void ASTPrinter::visit(ForStmt* node) {
    indent(); out << "ForStmt\n";
    depth++;
    if (node->init.decl) {
        indent(); out << "init:\n";
        depth++; node->init.decl->accept(this); depth--;
    } else if (node->init.expr) {
        indent(); out << "init:\n";
        depth++; node->init.expr->accept(this); depth--;
    }
    if (node->condition) {
        indent(); out << "cond:\n";
        depth++; node->condition->accept(this); depth--;
    }
    if (node->update) {
        indent(); out << "update:\n";
        depth++; node->update->accept(this); depth--;
    }
    indent(); out << "body:\n";
    depth++; node->body->accept(this); depth--;
    depth--;
}

void ASTPrinter::visit(ForRangeStmt* node) {
    indent();
    if (node->is_const) out << "const ";
    out << "ForRangeStmt '" << node->name << "' : " << typeStr(node->type) << "\n";
    depth++;
    indent(); out << "iterable:\n";
    depth++; node->iterable->accept(this); depth--;
    indent(); out << "body:\n";
    depth++; node->body->accept(this); depth--;
    depth--;
}

void ASTPrinter::visit(ReturnStmt* node) {
    indent(); out << "ReturnStmt\n";
    if (node->expr) {
        depth++;
        node->expr->accept(this);
        depth--;
    }
}

void ASTPrinter::visit(BreakStmt*) {
    indent(); out << "BreakStmt\n";
}

void ASTPrinter::visit(ContinueStmt*) {
    indent(); out << "ContinueStmt\n";
}

void ASTPrinter::visit(DeleteStmt* node) {
    indent(); out << "DeleteStmt" << (node->is_array ? "[]" : "") << "\n";
    depth++;
    node->expr->accept(this);
    depth--;
}

// ─── Global Declarations ──────────────────────────────────────────────────────

void ASTPrinter::visit(GlobalVarDecl* node) {
    indent();
    if (node->is_const) out << "const ";
    out << "GlobalVarDecl '" << node->name << "' : " << typeStr(node->type) << "\n";
    if (node->init) {
        depth++;
        node->init->accept(this);
        depth--;
    }
}

void ASTPrinter::visit(StructDecl* node) {
    indent(); out << "StructDecl '" << node->name << "'\n";
    depth++;
    for (auto& m : node->members) {
        indent(); out << "member '" << m.name << "' : " << typeStr(m.type) << "\n";
    }
    depth--;
}

void ASTPrinter::visit(FuncDecl* node) {
    indent(); out << "FuncDecl '" << node->name << "' -> " << typeStr(node->return_type) << "\n";
    depth++;
    for (auto& p : node->params) {
        indent();
        if (p.is_const) out << "const ";
        out << "param '" << p.name << "' : " << typeStr(p.type);
        if (p.is_ref) out << "&";
        out << "\n";
        if (p.default_val) {
            depth++;
            p.default_val->accept(this);
            depth--;
        }
    }
    node->body->accept(this);
    depth--;
}

void ASTPrinter::visit(TemplateFuncDecl* node) {
    indent(); out << "TemplateFuncDecl <typename " << node->template_param << ">\n";
    depth++;
    node->func->accept(this);
    depth--;
}
