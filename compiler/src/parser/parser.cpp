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

// true si el token actual puede comenzar un tipo
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
            if (n == TokenType::ID)  return true;  // MyType varName
            if (n == TokenType::LT)  return true;  // MyType<T>
            // MyType* varName  o  MyType& varName
            if ((n == TokenType::STAR || n == TokenType::AMP) &&
                peek(2).type == TokenType::ID) return true;
            return false;
        }
        default: return false;
    }
}

// true si el for actual (justo después de '(') es range-based
// un range-for tiene ':' antes de ';'
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

    TypeNode* type = parseType();
    std::string name = expect(TokenType::ID).lexeme;

    if (check(TokenType::LPAREN)) return parseFuncDecl(type, std::move(name));
    return parseGlobalVarDecl(type, std::move(name));
}

StructDecl* Parser::parseStructDecl() {
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
    return new StructDecl(std::move(name), std::move(members));
}

TemplateFuncDecl* Parser::parseTemplateFuncDecl() {
    expect(TokenType::KW_TEMPLATE);
    expect(TokenType::LT);
    expect(TokenType::KW_TYPENAME);
    std::string tparam = expect(TokenType::ID).lexeme;
    expect(TokenType::GT);

    TypeNode* ret = parseType();
    std::string name = expect(TokenType::ID).lexeme;
    FuncDecl* func = parseFuncDecl(ret, std::move(name));
    return new TemplateFuncDecl(std::move(tparam), func);
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

    // si el último modificador del tipo es &, es paso por referencia
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
    expect(TokenType::LBRACE);
    std::vector<Stmt*> stmts;
    while (!check(TokenType::RBRACE) && !check(TokenType::END))
        stmts.push_back(parseStmt());
    expect(TokenType::RBRACE);
    return new Block(std::move(stmts));
}

Stmt* Parser::parseStmt() {
    if (check(TokenType::LBRACE))    return parseBlock();
    if (check(TokenType::KW_IF))     return parseIfStmt();
    if (check(TokenType::KW_WHILE))  return parseWhileStmt();
    if (check(TokenType::KW_FOR))    return parseForStmt();
    if (check(TokenType::KW_RETURN)) return parseReturnStmt();
    if (check(TokenType::KW_DELETE)) return parseDeleteStmt();

    if (match(TokenType::KW_BREAK)) {
        expect(TokenType::SEMICOLON);
        return new BreakStmt();
    }
    if (match(TokenType::KW_CONTINUE)) {
        expect(TokenType::SEMICOLON);
        return new ContinueStmt();
    }

    // declaración de variable o sentencia de expresión
    if (isTypeStart()) {
        TypeNode* type = parseType();
        std::string name = expect(TokenType::ID).lexeme;
        return parseVarDeclStmt(type, std::move(name));
    }

    Expr* e = parseExpr();
    expect(TokenType::SEMICOLON);
    return new ExprStmt(e);
}

VarDeclStmt* Parser::parseVarDeclStmt(TypeNode* type, std::string name) {
    auto* node = new VarDeclStmt(type->is_const, type, std::move(name));

    if (check(TokenType::LBRACKET)) {
        // array: int arr[n][m] [= { init_list }]
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

    return new IfStmt(cond, then_b, else_b);
}

WhileStmt* Parser::parseWhileStmt() {
    expect(TokenType::KW_WHILE);
    expect(TokenType::LPAREN);
    Expr* cond = parseExpr();
    expect(TokenType::RPAREN);
    Block* body = parseBlock();
    return new WhileStmt(cond, body);
}

Stmt* Parser::parseForStmt() {
    expect(TokenType::KW_FOR);
    expect(TokenType::LPAREN);

    if (isRangeFor()) {
        // for ([const] Type id : Expr) Block
        TypeNode* type = parseType();
        std::string name = expect(TokenType::ID).lexeme;
        expect(TokenType::COLON);
        Expr* iterable = parseExpr();
        expect(TokenType::RPAREN);
        Block* body = parseBlock();
        return new ForRangeStmt(type->is_const, type, std::move(name), iterable, body);
    }

    // for (ForInit ; Expr ; Expr) Block
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

    Expr* cond = check(TokenType::SEMICOLON) ? nullptr : parseExpr();
    expect(TokenType::SEMICOLON);

    Expr* update = check(TokenType::RPAREN) ? nullptr : parseExpr();
    expect(TokenType::RPAREN);

    Block* body = parseBlock();
    return new ForStmt(init, cond, update, body);
}

ReturnStmt* Parser::parseReturnStmt() {
    expect(TokenType::KW_RETURN);
    Expr* val = check(TokenType::SEMICOLON) ? nullptr : parseExpr();
    expect(TokenType::SEMICOLON);
    return new ReturnStmt(val);
}

DeleteStmt* Parser::parseDeleteStmt() {
    expect(TokenType::KW_DELETE);
    bool is_array = false;
    if (match(TokenType::LBRACKET)) {
        expect(TokenType::RBRACKET);
        is_array = true;
    }
    Expr* expr = parseExpr();
    expect(TokenType::SEMICOLON);
    return new DeleteStmt(is_array, expr);
}

// ─── Expressions ──────────────────────────────────────────────────────────────

Expr* Parser::parseExpr()    { return parseAssign(); }

// Assign es right-associative: a = b = c  →  a = (b = c)
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
    return new AssignExpr(left, op, right);
}

Expr* Parser::parseLogicOr() {
    Expr* left = parseLogicAnd();
    while (match(TokenType::OR)) {
        Expr* right = parseLogicAnd();
        left = new BinaryExpr(left, BinaryOp::Or, right);
    }
    return left;
}

Expr* Parser::parseLogicAnd() {
    Expr* left = parseEquality();
    while (match(TokenType::AND)) {
        Expr* right = parseEquality();
        left = new BinaryExpr(left, BinaryOp::And, right);
    }
    return left;
}

Expr* Parser::parseEquality() {
    Expr* left = parseRelat();
    while (check(TokenType::EQ) || check(TokenType::NEQ)) {
        BinaryOp op = match(TokenType::EQ) ? BinaryOp::Eq : (consume(), BinaryOp::Neq);
        left = new BinaryExpr(left, op, parseRelat());
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
        left = new BinaryExpr(left, op, parseAdd());
    }
    return left;
}

Expr* Parser::parseAdd() {
    Expr* left = parseMul();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        BinaryOp op = match(TokenType::PLUS) ? BinaryOp::Add : (consume(), BinaryOp::Sub);
        left = new BinaryExpr(left, op, parseMul());
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
        left = new BinaryExpr(left, op, parseUnary());
    }
    return left;
}

Expr* Parser::parseUnary() {
    if (match(TokenType::MINUS))  return new UnaryExpr(UnaryOp::Neg,    parseUnary());
    if (match(TokenType::NOT))    return new UnaryExpr(UnaryOp::Not,    parseUnary());
    if (match(TokenType::TILDE))  return new UnaryExpr(UnaryOp::BitNot, parseUnary());
    if (match(TokenType::INC))    return new UnaryExpr(UnaryOp::PreInc, parseUnary());
    if (match(TokenType::DEC))    return new UnaryExpr(UnaryOp::PreDec, parseUnary());

    // * y & en contexto unario: desreferencia y dirección-de
    if (match(TokenType::STAR))   return new UnaryExpr(UnaryOp::Deref,  parseUnary());
    if (match(TokenType::AMP))    return new UnaryExpr(UnaryOp::AddrOf, parseUnary());

    if (match(TokenType::KW_STATIC_CAST)) {
        expect(TokenType::LT);
        TypeNode* type = parseType();
        expect(TokenType::GT);
        expect(TokenType::LPAREN);
        Expr* expr = parseExpr();
        expect(TokenType::RPAREN);
        return new CastExpr(type, expr);
    }

    if (match(TokenType::KW_NEW)) {
        TypeNode* type = parseType();
        if (match(TokenType::LBRACKET)) {
            Expr* size = parseExpr();
            expect(TokenType::RBRACKET);
            return new NewArrayExpr(type, size);
        }
        expect(TokenType::LPAREN);
        auto args = parseArgList();
        expect(TokenType::RPAREN);
        return new NewObjectExpr(type, std::move(args));
    }

    return parsePostfix();
}

Expr* Parser::parsePostfix() {
    Expr* base = parsePrimary();

    while (true) {
        if (match(TokenType::LBRACKET)) {
            Expr* idx = parseExpr();
            expect(TokenType::RBRACKET);
            base = new IndexExpr(base, idx);
        } else if (match(TokenType::LPAREN)) {
            auto args = parseArgList();
            expect(TokenType::RPAREN);
            base = new CallExpr(base, std::move(args));
        } else if (match(TokenType::DOT)) {
            std::string member = expect(TokenType::ID).lexeme;
            base = new MemberExpr(base, std::move(member), false);
        } else if (match(TokenType::ARROW)) {
            std::string member = expect(TokenType::ID).lexeme;
            base = new MemberExpr(base, std::move(member), true);
        } else if (match(TokenType::INC)) {
            base = new PostfixExpr(base, true);
        } else if (match(TokenType::DEC)) {
            base = new PostfixExpr(base, false);
        } else {
            break;
        }
    }

    return base;
}

Expr* Parser::parsePrimary() {
    if (check(TokenType::INT_LIT))
        return new IntLitExpr(std::stoll(consume().lexeme));

    if (check(TokenType::FLOAT_LIT))
        return new FloatLitExpr(std::stod(consume().lexeme));

    if (match(TokenType::KW_TRUE))  return new BoolLitExpr(true);
    if (match(TokenType::KW_FALSE)) return new BoolLitExpr(false);

    if (check(TokenType::CHAR_LIT))
        return new CharLitExpr(consume().lexeme);

    if (check(TokenType::STRING_LIT))
        return new StringLitExpr(consume().lexeme);

    if (check(TokenType::ID))
        return new IdExpr(consume().lexeme);

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
    expect(TokenType::LBRACKET);
    std::vector<CaptureItem> captures;

    if (!check(TokenType::RBRACKET)) {
        // [&] captura todo por referencia, [=] captura todo por valor
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
    return new LambdaExpr(std::move(captures), std::move(params), ret_type, body);
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
