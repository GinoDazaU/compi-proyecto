#include "parser.h"
#include <sstream>

// ─── Navigation ───────────────────────────────────────────────────────────────

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

Token& Parser::cur() {
    return tokens[pos];
}

Token& Parser::peek(int offset) {
    size_t i = pos + offset;
    return i < tokens.size() ? tokens[i] : tokens.back();
}

Token Parser::consume() {
    return tokens[pos++];
}

bool Parser::check(TokenType t) {
    return cur().type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) { consume(); return true; }
    return false;
}

Token Parser::expect(TokenType t) {
    if (!check(t)) error("Unexpected token '" + cur().lexeme + "'");
    return consume();
}

void Parser::error(const std::string& msg) {
    throw ParseError(msg, cur().line, cur().col);
}

// ─── Lookahead helpers ────────────────────────────────────────────────────────

bool Parser::isTypeStart() {
    switch (cur().type) {
        case TokenType::KW_CONST:
        case TokenType::KW_AUTO:
        case TokenType::KW_INT:  case TokenType::KW_LONG:
        case TokenType::KW_FLOAT: case TokenType::KW_DOUBLE:
        case TokenType::KW_BOOL: case TokenType::KW_CHAR:
        case TokenType::KW_VOID: case TokenType::KW_STRING:
            return true;
        case TokenType::ID: {
            TokenType n = peek().type;
            if (n == TokenType::ID)  return true;
            if (n == TokenType::LT)  return true;
            if ((n == TokenType::STAR || n == TokenType::AMP) &&
                peek(2).type == TokenType::ID) return true;
            return false;
        }
        default: return false;
    }
}

bool Parser::isRangeFor() {
    for (size_t i = pos; i < tokens.size(); i++) {
        if (tokens[i].type == TokenType::COLON)     return true;
        if (tokens[i].type == TokenType::SEMICOLON) return false;
        if (tokens[i].type == TokenType::RPAREN)    return false;
        if (tokens[i].type == TokenType::END)       return false;
    }
    return false;
}

// ─── Types ────────────────────────────────────────────────────────────────────

TypeNode* Parser::parseType() {
    auto* t = new TypeNode();

    if (match(TokenType::KW_CONST)) t->is_const = true;

    if (match(TokenType::KW_AUTO)) {
        t->is_auto = true;
    } else {
        switch (cur().type) {
            case TokenType::KW_INT: case TokenType::KW_LONG:
            case TokenType::KW_FLOAT: case TokenType::KW_DOUBLE:
            case TokenType::KW_BOOL: case TokenType::KW_CHAR:
            case TokenType::KW_VOID: case TokenType::KW_STRING:
                t->base = consume().lexeme;
                break;
            case TokenType::ID:
                t->base = consume().lexeme;
                if (match(TokenType::LT)) {
                    t->template_arg = parseType();
                    expect(TokenType::GT);
                }
                break;
            default:
                error("Expected type");
        }
    }

    while (check(TokenType::STAR) || check(TokenType::AMP)) {
        if (match(TokenType::STAR)) t->mods.push_back(PtrMod::Pointer);
        else { consume(); t->mods.push_back(PtrMod::Reference); }
    }

    return t;
}

// ─── Program ──────────────────────────────────────────────────────────────────

Program* Parser::parse() {
    std::vector<TopDecl*> decls;
    while (!check(TokenType::END))
        decls.push_back(parseTopDecl());
    return new Program(std::move(decls));
}

// ─── Top-level declarations ───────────────────────────────────────────────────

TopDecl* Parser::parseTopDecl() {
    if (check(TokenType::KW_TEMPLATE)) return parseTemplateFuncDecl();
    if (check(TokenType::KW_STRUCT))   return parseStructDecl();

    int ln = cur().line, cl = cur().col;
    TypeNode* type = parseType();
    std::string name = expect(TokenType::ID).lexeme;

    TopDecl* decl;
    if (check(TokenType::LPAREN)) decl = parseFuncDecl(type, std::move(name));
    else                          decl = parseGlobalVarDecl(type, std::move(name));
    decl->line = ln; decl->col = cl;
    return decl;
}

StructDecl* Parser::parseStructDecl() {
    int ln = cur().line, cl = cur().col;
    expect(TokenType::KW_STRUCT);
    std::string name = expect(TokenType::ID).lexeme;
    expect(TokenType::LBRACE);

    std::vector<MemberDecl> members;
    while (!check(TokenType::RBRACE) && !check(TokenType::END)) {
        TypeNode* mtype = parseType();
        std::string mname = expect(TokenType::ID).lexeme;
        expect(TokenType::SEMICOLON);
        members.push_back({mtype, std::move(mname)});
    }

    expect(TokenType::RBRACE);
    expect(TokenType::SEMICOLON);
    auto* node = new StructDecl(std::move(name), std::move(members));
    node->line = ln; node->col = cl;
    return node;
}

TemplateFuncDecl* Parser::parseTemplateFuncDecl() {
    int ln = cur().line, cl = cur().col;
    expect(TokenType::KW_TEMPLATE);
    expect(TokenType::LT);
    expect(TokenType::KW_TYPENAME);
    std::string tparam = expect(TokenType::ID).lexeme;
    expect(TokenType::GT);

    TypeNode* ret = parseType();
    std::string name = expect(TokenType::ID).lexeme;
    FuncDecl* func = parseFuncDecl(ret, std::move(name));
    auto* node = new TemplateFuncDecl(std::move(tparam), func);
    node->line = ln; node->col = cl;
    return node;
}

FuncDecl* Parser::parseFuncDecl(TypeNode* ret, std::string name) {
    expect(TokenType::LPAREN);
    auto params = parseParamList();
    expect(TokenType::RPAREN);
    Block* body = parseBlock();
    return new FuncDecl(ret, std::move(name), std::move(params), body);
}

GlobalVarDecl* Parser::parseGlobalVarDecl(TypeNode* type, std::string name) {
    Expr* init = nullptr;
    if (match(TokenType::ASSIGN)) init = parseExpr();
    expect(TokenType::SEMICOLON);
    return new GlobalVarDecl(type->is_const, type, std::move(name), init);
}

std::vector<Param> Parser::parseParamList() {
    std::vector<Param> params;
    if (check(TokenType::RPAREN)) return params;
    do {
        params.push_back(parseParam());
    } while (match(TokenType::COMMA));
    return params;
}

Param Parser::parseParam() {
    Param p;
    p.is_const    = false;
    p.is_ref      = false;
    p.default_val = nullptr;
    p.type = parseType();
    p.is_const = p.type->is_const;

    if (!p.type->mods.empty() && p.type->mods.back() == PtrMod::Reference) {
        p.is_ref = true;
        p.type->mods.pop_back();
    }

    p.name = expect(TokenType::ID).lexeme;
    if (match(TokenType::ASSIGN)) p.default_val = parseExpr();
    return p;
}

// ─── Statements ───────────────────────────────────────────────────────────────

Block* Parser::parseBlock() {
    int ln = cur().line, cl = cur().col;
    expect(TokenType::LBRACE);
    std::vector<Stmt*> stmts;
    while (!check(TokenType::RBRACE) && !check(TokenType::END))
        stmts.push_back(parseStmt());
    expect(TokenType::RBRACE);
    auto* node = new Block(std::move(stmts));
    node->line = ln; node->col = cl;
    return node;
}

Stmt* Parser::parseStmt() {
    if (check(TokenType::LBRACE))    return parseBlock();
    if (check(TokenType::KW_IF))     return parseIfStmt();
    if (check(TokenType::KW_WHILE))  return parseWhileStmt();
    if (check(TokenType::KW_FOR))    return parseForStmt();
    if (check(TokenType::KW_RETURN)) return parseReturnStmt();
    if (check(TokenType::KW_DELETE)) return parseDeleteStmt();

    if (check(TokenType::KW_BREAK)) {
        int ln = cur().line, cl = cur().col;
        consume();
        expect(TokenType::SEMICOLON);
        auto* node = new BreakStmt();
        node->line = ln; node->col = cl;
        return node;
    }
    if (check(TokenType::KW_CONTINUE)) {
        int ln = cur().line, cl = cur().col;
        consume();
        expect(TokenType::SEMICOLON);
        auto* node = new ContinueStmt();
        node->line = ln; node->col = cl;
        return node;
    }

    if (isTypeStart()) {
        int ln = cur().line, cl = cur().col;
        TypeNode* type = parseType();
        std::string name = expect(TokenType::ID).lexeme;
        auto* node = parseVarDeclStmt(type, std::move(name));
        node->line = ln; node->col = cl;
        return node;
    }

    int ln = cur().line, cl = cur().col;
    Expr* e = parseExpr();
    expect(TokenType::SEMICOLON);
    auto* node = new ExprStmt(e);
    node->line = ln; node->col = cl;
    return node;
}

VarDeclStmt* Parser::parseVarDeclStmt(TypeNode* type, std::string name) {
    auto* node = new VarDeclStmt(type->is_const, type, std::move(name));

    if (check(TokenType::LBRACKET)) {
        while (match(TokenType::LBRACKET)) {
            node->dimensions.push_back(parseExpr());
            expect(TokenType::RBRACKET);
        }
        if (match(TokenType::ASSIGN)) {
            expect(TokenType::LBRACE);
            node->init_list = parseInitList();
            expect(TokenType::RBRACE);
        }
    } else if (match(TokenType::ASSIGN)) {
        node->init = parseExpr();
    }

    expect(TokenType::SEMICOLON);
    return node;
}

IfStmt* Parser::parseIfStmt() {
    int ln = cur().line, cl = cur().col;
    expect(TokenType::KW_IF);
    expect(TokenType::LPAREN);
    Expr* cond = parseExpr();
    expect(TokenType::RPAREN);
    Block* then_b = parseBlock();

    Stmt* else_b = nullptr;
    if (match(TokenType::KW_ELSE)) {
        if (check(TokenType::KW_IF)) else_b = parseIfStmt();
        else                         else_b = parseBlock();
    }

    auto* node = new IfStmt(cond, then_b, else_b);
    node->line = ln; node->col = cl;
    return node;
}

WhileStmt* Parser::parseWhileStmt() {
    int ln = cur().line, cl = cur().col;
    expect(TokenType::KW_WHILE);
    expect(TokenType::LPAREN);
    Expr* cond = parseExpr();
    expect(TokenType::RPAREN);
    Block* body = parseBlock();
    auto* node = new WhileStmt(cond, body);
    node->line = ln; node->col = cl;
    return node;
}

Stmt* Parser::parseForStmt() {
    int ln = cur().line, cl = cur().col;
    expect(TokenType::KW_FOR);
    expect(TokenType::LPAREN);

    if (isRangeFor()) {
        TypeNode* type = parseType();
        std::string name = expect(TokenType::ID).lexeme;
        expect(TokenType::COLON);
        Expr* iterable = parseExpr();
        expect(TokenType::RPAREN);
        Block* body = parseBlock();
        auto* node = new ForRangeStmt(type->is_const, type, std::move(name), iterable, body);
        node->line = ln; node->col = cl;
        return node;
    }

    ForInit init;
    if (!check(TokenType::SEMICOLON)) {
        if (isTypeStart()) {
            TypeNode* type = parseType();
            std::string name = expect(TokenType::ID).lexeme;
            init.decl = new VarDeclStmt(type->is_const, type, std::move(name));
            if (match(TokenType::ASSIGN)) init.decl->init = parseExpr();
        } else {
            init.expr = parseExpr();
        }
    }
    expect(TokenType::SEMICOLON);

    Expr* cond   = check(TokenType::SEMICOLON) ? nullptr : parseExpr();
    expect(TokenType::SEMICOLON);
    Expr* update = check(TokenType::RPAREN)    ? nullptr : parseExpr();
    expect(TokenType::RPAREN);

    Block* body = parseBlock();
    auto* node = new ForStmt(init, cond, update, body);
    node->line = ln; node->col = cl;
    return node;
}

ReturnStmt* Parser::parseReturnStmt() {
    int ln = cur().line, cl = cur().col;
    expect(TokenType::KW_RETURN);
    Expr* val = check(TokenType::SEMICOLON) ? nullptr : parseExpr();
    expect(TokenType::SEMICOLON);
    auto* node = new ReturnStmt(val);
    node->line = ln; node->col = cl;
    return node;
}

DeleteStmt* Parser::parseDeleteStmt() {
    int ln = cur().line, cl = cur().col;
    expect(TokenType::KW_DELETE);
    bool is_array = false;
    if (match(TokenType::LBRACKET)) {
        expect(TokenType::RBRACKET);
        is_array = true;
    }
    Expr* expr = parseExpr();
    expect(TokenType::SEMICOLON);
    auto* node = new DeleteStmt(is_array, expr);
    node->line = ln; node->col = cl;
    return node;
}

// ─── Expressions ──────────────────────────────────────────────────────────────

Expr* Parser::parseExpr() { return parseAssign(); }

Expr* Parser::parseAssign() {
    Expr* left = parseLogicOr();

    AssignOp op;
    switch (cur().type) {
        case TokenType::ASSIGN:          op = AssignOp::Assign;      break;
        case TokenType::PLUS_ASSIGN:     op = AssignOp::PlusAssign;  break;
        case TokenType::MINUS_ASSIGN:    op = AssignOp::MinusAssign; break;
        case TokenType::STAR_ASSIGN:     op = AssignOp::MulAssign;   break;
        case TokenType::SLASH_ASSIGN:    op = AssignOp::DivAssign;   break;
        case TokenType::PERCENT_ASSIGN:  op = AssignOp::ModAssign;   break;
        case TokenType::AMP_ASSIGN:      op = AssignOp::AndAssign;   break;
        case TokenType::PIPE_ASSIGN:     op = AssignOp::OrAssign;    break;
        default: return left;
    }
    consume();
    Expr* right = parseAssign();
    auto* node = new AssignExpr(left, op, right);
    node->line = left->line; node->col = left->col;
    return node;
}

Expr* Parser::parseLogicOr() {
    Expr* left = parseLogicAnd();
    while (match(TokenType::OR)) {
        Expr* right = parseLogicAnd();
        auto* node = new BinaryExpr(left, BinaryOp::Or, right);
        node->line = left->line; node->col = left->col;
        left = node;
    }
    return left;
}

Expr* Parser::parseLogicAnd() {
    Expr* left = parseEquality();
    while (match(TokenType::AND)) {
        Expr* right = parseEquality();
        auto* node = new BinaryExpr(left, BinaryOp::And, right);
        node->line = left->line; node->col = left->col;
        left = node;
    }
    return left;
}

Expr* Parser::parseEquality() {
    Expr* left = parseRelat();
    while (check(TokenType::EQ) || check(TokenType::NEQ)) {
        BinaryOp op = match(TokenType::EQ) ? BinaryOp::Eq : (consume(), BinaryOp::Neq);
        Expr* right = parseRelat();
        auto* node = new BinaryExpr(left, op, right);
        node->line = left->line; node->col = left->col;
        left = node;
    }
    return left;
}

Expr* Parser::parseRelat() {
    Expr* left = parseAdd();
    while (true) {
        BinaryOp op;
        if      (match(TokenType::LT))  op = BinaryOp::Lt;
        else if (match(TokenType::GT))  op = BinaryOp::Gt;
        else if (match(TokenType::LEQ)) op = BinaryOp::Leq;
        else if (match(TokenType::GEQ)) op = BinaryOp::Geq;
        else break;
        Expr* right = parseAdd();
        auto* node = new BinaryExpr(left, op, right);
        node->line = left->line; node->col = left->col;
        left = node;
    }
    return left;
}

Expr* Parser::parseAdd() {
    Expr* left = parseMul();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        BinaryOp op = match(TokenType::PLUS) ? BinaryOp::Add : (consume(), BinaryOp::Sub);
        Expr* right = parseMul();
        auto* node = new BinaryExpr(left, op, right);
        node->line = left->line; node->col = left->col;
        left = node;
    }
    return left;
}

Expr* Parser::parseMul() {
    Expr* left = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        BinaryOp op;
        if      (match(TokenType::STAR))    op = BinaryOp::Mul;
        else if (match(TokenType::SLASH))   op = BinaryOp::Div;
        else                                { consume(); op = BinaryOp::Mod; }
        Expr* right = parseUnary();
        auto* node = new BinaryExpr(left, op, right);
        node->line = left->line; node->col = left->col;
        left = node;
    }
    return left;
}

Expr* Parser::parseUnary() {
    int ln = cur().line, cl = cur().col;

    if (match(TokenType::MINUS))  { auto* n = new UnaryExpr(UnaryOp::Neg,    parseUnary()); n->line=ln; n->col=cl; return n; }
    if (match(TokenType::NOT))    { auto* n = new UnaryExpr(UnaryOp::Not,    parseUnary()); n->line=ln; n->col=cl; return n; }
    if (match(TokenType::TILDE))  { auto* n = new UnaryExpr(UnaryOp::BitNot, parseUnary()); n->line=ln; n->col=cl; return n; }
    if (match(TokenType::INC))    { auto* n = new UnaryExpr(UnaryOp::PreInc, parseUnary()); n->line=ln; n->col=cl; return n; }
    if (match(TokenType::DEC))    { auto* n = new UnaryExpr(UnaryOp::PreDec, parseUnary()); n->line=ln; n->col=cl; return n; }
    if (match(TokenType::STAR))   { auto* n = new UnaryExpr(UnaryOp::Deref,  parseUnary()); n->line=ln; n->col=cl; return n; }
    if (match(TokenType::AMP))    { auto* n = new UnaryExpr(UnaryOp::AddrOf, parseUnary()); n->line=ln; n->col=cl; return n; }

    if (match(TokenType::KW_STATIC_CAST)) {
        expect(TokenType::LT);
        TypeNode* type = parseType();
        expect(TokenType::GT);
        expect(TokenType::LPAREN);
        Expr* expr = parseExpr();
        expect(TokenType::RPAREN);
        auto* n = new CastExpr(type, expr);
        n->line = ln; n->col = cl;
        return n;
    }

    if (match(TokenType::KW_NEW)) {
        TypeNode* type = parseType();
        if (match(TokenType::LBRACKET)) {
            Expr* size = parseExpr();
            expect(TokenType::RBRACKET);
            auto* n = new NewArrayExpr(type, size);
            n->line = ln; n->col = cl;
            return n;
        }
        expect(TokenType::LPAREN);
        auto args = parseArgList();
        expect(TokenType::RPAREN);
        auto* n = new NewObjectExpr(type, std::move(args));
        n->line = ln; n->col = cl;
        return n;
    }

    return parsePostfix();
}

Expr* Parser::parsePostfix() {
    Expr* base = parsePrimary();

    while (true) {
        if (match(TokenType::LBRACKET)) {
            Expr* idx = parseExpr();
            expect(TokenType::RBRACKET);
            auto* n = new IndexExpr(base, idx);
            n->line = base->line; n->col = base->col;
            base = n;
        } else if (match(TokenType::LPAREN)) {
            auto args = parseArgList();
            expect(TokenType::RPAREN);
            auto* n = new CallExpr(base, std::move(args));
            n->line = base->line; n->col = base->col;
            base = n;
        } else if (match(TokenType::DOT)) {
            std::string member = expect(TokenType::ID).lexeme;
            auto* n = new MemberExpr(base, std::move(member), false);
            n->line = base->line; n->col = base->col;
            base = n;
        } else if (match(TokenType::ARROW)) {
            std::string member = expect(TokenType::ID).lexeme;
            auto* n = new MemberExpr(base, std::move(member), true);
            n->line = base->line; n->col = base->col;
            base = n;
        } else if (match(TokenType::INC)) {
            auto* n = new PostfixExpr(base, true);
            n->line = base->line; n->col = base->col;
            base = n;
        } else if (match(TokenType::DEC)) {
            auto* n = new PostfixExpr(base, false);
            n->line = base->line; n->col = base->col;
            base = n;
        } else {
            break;
        }
    }

    return base;
}

Expr* Parser::parsePrimary() {
    int ln = cur().line, cl = cur().col;

    if (check(TokenType::INT_LIT)) {
        auto* n = new IntLitExpr(std::stoll(consume().lexeme));
        n->line = ln; n->col = cl; return n;
    }
    if (check(TokenType::FLOAT_LIT)) {
        auto* n = new FloatLitExpr(std::stod(consume().lexeme));
        n->line = ln; n->col = cl; return n;
    }
    if (match(TokenType::KW_TRUE))  { auto* n = new BoolLitExpr(true);  n->line=ln; n->col=cl; return n; }
    if (match(TokenType::KW_FALSE)) { auto* n = new BoolLitExpr(false); n->line=ln; n->col=cl; return n; }

    if (check(TokenType::CHAR_LIT)) {
        auto* n = new CharLitExpr(consume().lexeme);
        n->line = ln; n->col = cl; return n;
    }
    if (check(TokenType::STRING_LIT)) {
        auto* n = new StringLitExpr(consume().lexeme);
        n->line = ln; n->col = cl; return n;
    }
    if (check(TokenType::ID)) {
        auto* n = new IdExpr(consume().lexeme);
        n->line = ln; n->col = cl; return n;
    }
    if (match(TokenType::LPAREN)) {
        Expr* e = parseExpr();
        expect(TokenType::RPAREN);
        return e;
    }
    if (check(TokenType::LBRACKET))
        return parseLambda();

    error("Expected expression");
}

LambdaExpr* Parser::parseLambda() {
    int ln = cur().line, cl = cur().col;
    expect(TokenType::LBRACKET);
    std::vector<CaptureItem> captures;

    if (!check(TokenType::RBRACKET)) {
        if (check(TokenType::AMP) && peek().type == TokenType::RBRACKET) {
            consume();
            captures.push_back({true, ""});
        } else if (check(TokenType::ASSIGN) && peek().type == TokenType::RBRACKET) {
            consume();
            captures.push_back({false, ""});
        } else {
            do {
                CaptureItem item;
                item.is_ref = match(TokenType::AMP);
                item.name   = expect(TokenType::ID).lexeme;
                captures.push_back(item);
            } while (match(TokenType::COMMA));
        }
    }

    expect(TokenType::RBRACKET);
    expect(TokenType::LPAREN);
    auto params = parseParamList();
    expect(TokenType::RPAREN);

    TypeNode* ret_type = nullptr;
    if (match(TokenType::ARROW)) ret_type = parseType();

    Block* body = parseBlock();
    auto* node = new LambdaExpr(std::move(captures), std::move(params), ret_type, body);
    node->line = ln; node->col = cl;
    return node;
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

std::vector<Expr*> Parser::parseArgList() {
    std::vector<Expr*> args;
    if (!check(TokenType::RPAREN)) {
        do { args.push_back(parseExpr()); } while (match(TokenType::COMMA));
    }
    return args;
}

std::vector<Expr*> Parser::parseInitList() {
    std::vector<Expr*> items;
    if (!check(TokenType::RBRACE)) {
        do { items.push_back(parseExpr()); } while (match(TokenType::COMMA));
    }
    return items;
}
