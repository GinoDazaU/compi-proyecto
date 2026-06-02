#include "ast_json_printer.h"
#include "ast.h"

// ─── Helpers ──────────────────────────────────────────────────────────────────

void ASTJsonPrinter::indent() {
    for (int i = 0; i < depth; i++) out << "  ";
}

void ASTJsonPrinter::printString(const std::string& s) {
    out << "\"";
    for (char c : s) {
        if (c == '"') out << "\\\"";
        else if (c == '\\') out << "\\\\";
        else if (c == '\n') out << "\\n";
        else if (c == '\t') out << "\\t";
        else if (c == '\r') out << "\\r";
        else out << c;
    }
    out << "\"";
}

std::string ASTJsonPrinter::typeStr(TypeNode* t) {
    if (!t) return "";
    if (t->is_auto) return "auto";
    std::string s;
    if (t->is_const) s += "const ";
    s += t->base;
    if (t->template_arg) s += "<" + typeStr(t->template_arg) + ">";
    for (auto mod : t->mods)
        s += (mod == PtrMod::Pointer ? "*" : "&");
    return s;
}

const char* ASTJsonPrinter::binaryOpStr(BinaryOp op) {
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

const char* ASTJsonPrinter::assignOpStr(AssignOp op) {
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

const char* ASTJsonPrinter::unaryOpStr(UnaryOp op) {
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

void ASTJsonPrinter::visit(Program* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"Program\",\n";
    indent(); out << "\"decls\": [\n";
    depth++;
    for (size_t i = 0; i < node->decls.size(); i++) {
        node->decls[i]->accept(this);
        if (i + 1 < node->decls.size()) {
            out << ",\n";
        } else {
            out << "\n";
        }
    }
    depth--;
    indent(); out << "]\n";
    depth--;
    indent(); out << "}\n";
}

// ─── Literals ─────────────────────────────────────────────────────────────────

void ASTJsonPrinter::visit(IntLitExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"IntLitExpr\",\n";
    indent(); out << "\"value\": " << node->value << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(FloatLitExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"FloatLitExpr\",\n";
    indent(); out << "\"value\": " << node->value << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(BoolLitExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"BoolLitExpr\",\n";
    indent(); out << "\"value\": " << (node->value ? "true" : "false") << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(CharLitExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"CharLitExpr\",\n";
    indent(); out << "\"value\": "; printString(node->value); out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(StringLitExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"StringLitExpr\",\n";
    indent(); out << "\"value\": "; printString(node->value); out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(IdExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"IdExpr\",\n";
    indent(); out << "\"name\": "; printString(node->name); out << "\n";
    depth--;
    indent(); out << "}";
}

// ─── Expressions ──────────────────────────────────────────────────────────────

void ASTJsonPrinter::visit(BinaryExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"BinaryExpr\",\n";
    indent(); out << "\"op\": "; printString(binaryOpStr(node->op)); out << ",\n";
    indent(); out << "\"left\":\n";
    node->left->accept(this);
    out << ",\n";
    indent(); out << "\"right\":\n";
    node->right->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(UnaryExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"UnaryExpr\",\n";
    indent(); out << "\"op\": "; printString(unaryOpStr(node->op)); out << ",\n";
    indent(); out << "\"expr\":\n";
    node->expr->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(AssignExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"AssignExpr\",\n";
    indent(); out << "\"op\": "; printString(assignOpStr(node->op)); out << ",\n";
    indent(); out << "\"left\":\n";
    node->left->accept(this);
    out << ",\n";
    indent(); out << "\"right\":\n";
    node->right->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(CastExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"CastExpr\",\n";
    indent(); out << "\"type\": "; printString(typeStr(node->type)); out << ",\n";
    indent(); out << "\"expr\":\n";
    node->expr->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(NewArrayExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"NewArrayExpr\",\n";
    indent(); out << "\"type\": "; printString(typeStr(node->type)); out << ",\n";
    indent(); out << "\"size\":\n";
    node->size->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(NewObjectExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"NewObjectExpr\",\n";
    indent(); out << "\"type\": "; printString(typeStr(node->type)); out << ",\n";
    indent(); out << "\"args\": [\n";
    depth++;
    for (size_t i = 0; i < node->args.size(); i++) {
        node->args[i]->accept(this);
        if (i + 1 < node->args.size()) {
            out << ",\n";
        } else {
            out << "\n";
        }
    }
    depth--;
    indent(); out << "]\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(IndexExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"IndexExpr\",\n";
    indent(); out << "\"base\":\n";
    node->base->accept(this);
    out << ",\n";
    indent(); out << "\"index\":\n";
    node->index->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(CallExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"CallExpr\",\n";
    indent(); out << "\"callee\":\n";
    node->callee->accept(this);
    out << ",\n";
    indent(); out << "\"args\": [\n";
    depth++;
    for (size_t i = 0; i < node->args.size(); i++) {
        node->args[i]->accept(this);
        if (i + 1 < node->args.size()) {
            out << ",\n";
        } else {
            out << "\n";
        }
    }
    depth--;
    indent(); out << "]\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(MemberExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"MemberExpr\",\n";
    indent(); out << "\"member\": "; printString(node->member); out << ",\n";
    indent(); out << "\"is_arrow\": " << (node->is_arrow ? "true" : "false") << ",\n";
    indent(); out << "\"base\":\n";
    node->base->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(PostfixExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"PostfixExpr\",\n";
    indent(); out << "\"is_inc\": " << (node->is_inc ? "true" : "false") << ",\n";
    indent(); out << "\"base\":\n";
    node->base->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(LambdaExpr* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"LambdaExpr\",\n";
    
    // Captures
    indent(); out << "\"captures\": [\n";
    depth++;
    for (size_t i = 0; i < node->captures.size(); i++) {
        indent(); out << "{\n";
        depth++;
        indent(); out << "\"is_ref\": " << (node->captures[i].is_ref ? "true" : "false") << ",\n";
        indent(); out << "\"name\": "; printString(node->captures[i].name); out << "\n";
        depth--;
        indent(); out << "}";
        if (i + 1 < node->captures.size()) out << ",\n";
        else out << "\n";
    }
    depth--;
    indent(); out << "],\n";

    // Params
    indent(); out << "\"params\": [\n";
    depth++;
    for (size_t i = 0; i < node->params.size(); i++) {
        indent(); out << "{\n";
        depth++;
        indent(); out << "\"is_const\": " << (node->params[i].is_const ? "true" : "false") << ",\n";
        indent(); out << "\"type\": "; printString(typeStr(node->params[i].type)); out << ",\n";
        indent(); out << "\"is_ref\": " << (node->params[i].is_ref ? "true" : "false") << ",\n";
        indent(); out << "\"name\": "; printString(node->params[i].name);
        if (node->params[i].default_val) {
            out << ",\n";
            indent(); out << "\"default_val\":\n";
            node->params[i].default_val->accept(this);
            out << "\n";
        } else {
            out << "\n";
        }
        depth--;
        indent(); out << "}";
        if (i + 1 < node->params.size()) out << ",\n";
        else out << "\n";
    }
    depth--;
    indent(); out << "],\n";

    // Return type
    indent(); out << "\"return_type\": ";
    if (node->return_type) {
        printString(typeStr(node->return_type));
    } else {
        out << "null";
    }
    out << ",\n";

    // Body
    indent(); out << "\"body\":\n";
    node->body->accept(this);
    out << "\n";

    depth--;
    indent(); out << "}";
}

// ─── Statements ───────────────────────────────────────────────────────────────

void ASTJsonPrinter::visit(Block* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"Block\",\n";
    indent(); out << "\"stmts\": [\n";
    depth++;
    for (size_t i = 0; i < node->stmts.size(); i++) {
        node->stmts[i]->accept(this);
        if (i + 1 < node->stmts.size()) {
            out << ",\n";
        } else {
            out << "\n";
        }
    }
    depth--;
    indent(); out << "]\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(VarDeclStmt* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"VarDeclStmt\",\n";
    indent(); out << "\"is_const\": " << (node->is_const ? "true" : "false") << ",\n";
    indent(); out << "\"type\": "; printString(typeStr(node->type)); out << ",\n";
    indent(); out << "\"name\": "; printString(node->name);
    
    if (node->init) {
        out << ",\n";
        indent(); out << "\"init\":\n";
        node->init->accept(this);
    }
    
    if (!node->dimensions.empty()) {
        out << ",\n";
        indent(); out << "\"dimensions\": [\n";
        depth++;
        for (size_t i = 0; i < node->dimensions.size(); i++) {
            node->dimensions[i]->accept(this);
            if (i + 1 < node->dimensions.size()) out << ",\n";
            else out << "\n";
        }
        depth--;
        indent(); out << "]";
    }

    if (!node->init_list.empty()) {
        out << ",\n";
        indent(); out << "\"init_list\": [\n";
        depth++;
        for (size_t i = 0; i < node->init_list.size(); i++) {
            node->init_list[i]->accept(this);
            if (i + 1 < node->init_list.size()) out << ",\n";
            else out << "\n";
        }
        depth--;
        indent(); out << "]";
    }

    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(ExprStmt* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"ExprStmt\",\n";
    indent(); out << "\"expr\":\n";
    node->expr->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(IfStmt* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"IfStmt\",\n";
    indent(); out << "\"condition\":\n";
    node->condition->accept(this);
    out << ",\n";
    indent(); out << "\"then_branch\":\n";
    node->then_branch->accept(this);
    if (node->else_branch) {
        out << ",\n";
        indent(); out << "\"else_branch\":\n";
        node->else_branch->accept(this);
    }
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(WhileStmt* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"WhileStmt\",\n";
    indent(); out << "\"condition\":\n";
    node->condition->accept(this);
    out << ",\n";
    indent(); out << "\"body\":\n";
    node->body->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(ForStmt* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"ForStmt\",\n";
    
    // ForInit
    indent(); out << "\"init\": ";
    if (node->init.decl) {
        out << "\n";
        node->init.decl->accept(this);
    } else if (node->init.expr) {
        out << "\n";
        node->init.expr->accept(this);
    } else {
        out << "null";
    }
    out << ",\n";

    // Condition
    indent(); out << "\"condition\": ";
    if (node->condition) {
        out << "\n";
        node->condition->accept(this);
    } else {
        out << "null";
    }
    out << ",\n";

    // Update
    indent(); out << "\"update\": ";
    if (node->update) {
        out << "\n";
        node->update->accept(this);
    } else {
        out << "null";
    }
    out << ",\n";

    // Body
    indent(); out << "\"body\":\n";
    node->body->accept(this);
    out << "\n";

    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(ForRangeStmt* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"ForRangeStmt\",\n";
    indent(); out << "\"is_const\": " << (node->is_const ? "true" : "false") << ",\n";
    indent(); out << "\"type\": "; printString(typeStr(node->type)); out << ",\n";
    indent(); out << "\"name\": "; printString(node->name); out << ",\n";
    indent(); out << "\"iterable\":\n";
    node->iterable->accept(this);
    out << ",\n";
    indent(); out << "\"body\":\n";
    node->body->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(ReturnStmt* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"ReturnStmt\"";
    if (node->expr) {
        out << ",\n";
        indent(); out << "\"expr\":\n";
        node->expr->accept(this);
        out << "\n";
    } else {
        out << "\n";
    }
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(BreakStmt*) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"BreakStmt\"\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(ContinueStmt*) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"ContinueStmt\"\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(DeleteStmt* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"DeleteStmt\",\n";
    indent(); out << "\"is_array\": " << (node->is_array ? "true" : "false") << ",\n";
    indent(); out << "\"expr\":\n";
    node->expr->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}

// ─── Global Declarations ──────────────────────────────────────────────────────

void ASTJsonPrinter::visit(GlobalVarDecl* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"GlobalVarDecl\",\n";
    indent(); out << "\"is_const\": " << (node->is_const ? "true" : "false") << ",\n";
    indent(); out << "\"type\": "; printString(typeStr(node->type)); out << ",\n";
    indent(); out << "\"name\": "; printString(node->name);
    if (node->init) {
        out << ",\n";
        indent(); out << "\"init\":\n";
        node->init->accept(this);
        out << "\n";
    } else {
        out << "\n";
    }
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(StructDecl* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"StructDecl\",\n";
    indent(); out << "\"name\": "; printString(node->name); out << ",\n";
    indent(); out << "\"members\": [\n";
    depth++;
    for (size_t i = 0; i < node->members.size(); i++) {
        indent(); out << "{\n";
        depth++;
        indent(); out << "\"name\": "; printString(node->members[i].name); out << ",\n";
        indent(); out << "\"type\": "; printString(typeStr(node->members[i].type)); out << "\n";
        depth--;
        indent(); out << "}";
        if (i + 1 < node->members.size()) out << ",\n";
        else out << "\n";
    }
    depth--;
    indent(); out << "]\n";
    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(FuncDecl* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"FuncDecl\",\n";
    indent(); out << "\"name\": "; printString(node->name); out << ",\n";
    indent(); out << "\"return_type\": "; printString(typeStr(node->return_type)); out << ",\n";
    
    // Params
    indent(); out << "\"params\": [\n";
    depth++;
    for (size_t i = 0; i < node->params.size(); i++) {
        indent(); out << "{\n";
        depth++;
        indent(); out << "\"is_const\": " << (node->params[i].is_const ? "true" : "false") << ",\n";
        indent(); out << "\"type\": "; printString(typeStr(node->params[i].type)); out << ",\n";
        indent(); out << "\"is_ref\": " << (node->params[i].is_ref ? "true" : "false") << ",\n";
        indent(); out << "\"name\": "; printString(node->params[i].name);
        if (node->params[i].default_val) {
            out << ",\n";
            indent(); out << "\"default_val\":\n";
            node->params[i].default_val->accept(this);
            out << "\n";
        } else {
            out << "\n";
        }
        depth--;
        indent(); out << "}";
        if (i + 1 < node->params.size()) out << ",\n";
        else out << "\n";
    }
    depth--;
    indent(); out << "],\n";

    // Body
    indent(); out << "\"body\":\n";
    node->body->accept(this);
    out << "\n";

    depth--;
    indent(); out << "}";
}

void ASTJsonPrinter::visit(TemplateFuncDecl* node) {
    indent(); out << "{\n";
    depth++;
    indent(); out << "\"node\": \"TemplateFuncDecl\",\n";
    indent(); out << "\"template_param\": "; printString(node->template_param); out << ",\n";
    indent(); out << "\"func\":\n";
    node->func->accept(this);
    out << "\n";
    depth--;
    indent(); out << "}";
}
