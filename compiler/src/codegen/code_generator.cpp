#include "code_generator.h"
#include <functional>
#include <sstream>
#include <iomanip>

// ─── Helper local: código numérico de un CharLitExpr ─────────────────────────
// El lexema llega con comillas incluidas: 'a' o '\n'
static long charToCode(const std::string& lex) {
    if (lex.size() < 3) return 0;
    std::string inner = lex.substr(1, lex.size() - 2);
    if (inner.size() >= 2 && inner[0] == '\\') {
        switch (inner[1]) {
            case 'n':  return '\n';
            case 't':  return '\t';
            case 'r':  return '\r';
            case '0':  return '\0';
            case '\\': return '\\';
            case '\'': return '\'';
            case '"':  return '"';
            default:   return static_cast<unsigned char>(inner[1]);
        }
    }
    return static_cast<unsigned char>(inner[0]);
}

// ═════════════════════════════════════════════════════════════════════════════
// Construcción y entrada principal
// ═════════════════════════════════════════════════════════════════════════════

CodeGenerator::CodeGenerator(std::ostream& out) : out_(out) {}

void CodeGenerator::gencode(Program* program) {
    firstPass(program);

    // Emitimos el .text a un buffer primero: durante el recorrido se descubren
    // los float/string literals que van en .rodata, y esa sección debe estar
    // declarada antes de usar los labels. Redirigimos temporalmente out_.
    std::ostringstream body;
    std::streambuf* prev = out_.rdbuf(body.rdbuf());
    program->accept(this);  // → visit(Program)
    out_.rdbuf(prev);

    emitDataSection();
    emitRodataSection();
    out_ << ".text\n";
    out_ << body.str();

    // El stack no es ejecutable (requerido en Linux moderno)
    out_ << ".section .note.GNU-stack,\"\",@progbits\n";
}

// ═════════════════════════════════════════════════════════════════════════════
// Primera pasada: tamaños de frame y layouts de struct
// ═════════════════════════════════════════════════════════════════════════════

void CodeGenerator::firstPass(Program* program) {
    for (auto d : program->decls) {
        if (auto* f = dynamic_cast<FuncDecl*>(d)) {
            frame_sizes_[f->name] = frameSize(f);
        }
        // TODO: StructDecl → buildStructInfo(s);
        // TODO: TemplateFuncDecl, GlobalVarDecl
    }
}

// Cuenta variables locales (8 bytes c/u por ahora) + parámetros, redondea a 16.
int CodeGenerator::frameSize(FuncDecl* f) {
    int slots = static_cast<int>(f->params.size());

    std::function<void(Stmt*)> countStmt = [&](Stmt* s) {
        if (!s) return;
        if (dynamic_cast<VarDeclStmt*>(s)) {
            slots += 1;
        } else if (auto* b = dynamic_cast<Block*>(s)) {
            for (auto inner : b->stmts) countStmt(inner);
        } else if (auto* i = dynamic_cast<IfStmt*>(s)) {
            countStmt(i->then_branch);
            countStmt(i->else_branch);
        } else if (auto* w = dynamic_cast<WhileStmt*>(s)) {
            countStmt(w->body);
        } else if (auto* fr = dynamic_cast<ForStmt*>(s)) {
            if (fr->init.decl) slots += 1;
            countStmt(fr->body);
        } else if (auto* rg = dynamic_cast<ForRangeStmt*>(s)) {
            slots += 1;
            countStmt(rg->body);
        }
    };
    countStmt(f->body);

    int bytes = slots * 8;
    if (bytes % 16 != 0) bytes += 16 - (bytes % 16);
    return bytes;
}

void CodeGenerator::buildStructInfo(StructDecl* /*s*/) {
    // TODO: calcular offsets y tamaño de cada struct
}

// ═════════════════════════════════════════════════════════════════════════════
// Helpers de emisión
// ═════════════════════════════════════════════════════════════════════════════

int CodeGenerator::nextLabel() { return label_counter_++; }

std::string CodeGenerator::newStrLabel() {
    return "__str_" + std::to_string(str_counter_++);
}

std::string CodeGenerator::floatLabel(double v) {
    for (auto& [lbl, val] : float_literals_)
        if (val == v) return lbl;
    std::string lbl = "__float_" + std::to_string(float_counter_++);
    float_literals_.push_back({lbl, v});
    return lbl;
}

std::string CodeGenerator::strLabel(const std::string& lexeme) {
    for (auto& [lbl, val] : string_literals_)
        if (val == lexeme) return lbl;
    std::string lbl = newStrLabel();
    string_literals_.push_back({lbl, lexeme});
    return lbl;
}

// Carga la variable en offset(%rbp) al registro de su tipo.
//   float → %xmm0 (movsd) ; bool/char → %al + zero-extend a %rax ; resto → %rax
void CodeGenerator::emitLoad(const SemType& t, int offset) {
    if (t.base == "float" && !t.hasPointer()) {
        out_ << "    movsd " << offset << "(%rbp), %xmm0\n";
    } else if ((t.base == "bool" || t.base == "char") && !t.hasPointer()) {
        out_ << "    movb " << offset << "(%rbp), %al\n";
        out_ << "    movzbq %al, %rax\n";
    } else {
        out_ << "    movq " << offset << "(%rbp), %rax\n";
    }
}

// Guarda el resultado actual (en %rax o %xmm0) en offset(%rbp) según el tipo.
void CodeGenerator::emitStore(const SemType& t, int offset) {
    if (t.base == "float" && !t.hasPointer()) {
        out_ << "    movsd %xmm0, " << offset << "(%rbp)\n";
    } else if ((t.base == "bool" || t.base == "char") && !t.hasPointer()) {
        out_ << "    movb %al, " << offset << "(%rbp)\n";
    } else {
        out_ << "    movq %rax, " << offset << "(%rbp)\n";
    }
}
void CodeGenerator::emitPush(const SemType& /*t*/)                  { /* TODO */ }
void CodeGenerator::emitPop(const SemType& /*t*/, const std::string& /*reg*/) { /* TODO */ }

void CodeGenerator::emitDataSection() {
    out_ << ".data\n";
    out_ << "__fmt_int:   .string \"%ld\"\n";
    out_ << "__fmt_float: .string \"%lf\"\n";
    out_ << "__fmt_char:  .string \"%c\"\n";
    out_ << "__fmt_str:   .string \"%s\"\n";
    out_ << "__fmt_nl:    .string \"\\n\"\n";
}

void CodeGenerator::emitRodataSection() {
    if (float_literals_.empty() && string_literals_.empty()) return;
    out_ << ".section .rodata\n";
    for (auto& [lbl, val] : float_literals_)
        out_ << lbl << ": .double " << std::setprecision(17) << val << "\n";
    for (auto& [lbl, val] : string_literals_)
        out_ << lbl << ": .string " << val << "\n";  // val incluye comillas
}

// ═════════════════════════════════════════════════════════════════════════════
// Programa y declaraciones globales
// ═════════════════════════════════════════════════════════════════════════════

void CodeGenerator::visit(Program* node) {
    for (auto d : node->decls) d->accept(this);
}

void CodeGenerator::visit(FuncDecl* node) {
    current_func_ = node->name;
    offset_       = -8;
    env_.enterScope();

    out_ << ".globl " << node->name << "\n";
    out_ << node->name << ":\n";
    out_ << "    pushq %rbp\n";
    out_ << "    movq %rsp, %rbp\n";

    int frame = frame_sizes_[node->name];
    if (frame > 0) out_ << "    subq $" << frame << ", %rsp\n";

    // Guardar parámetros (solo int/ptr por ahora)
    static const char* argRegs[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    int i = 0;
    for (auto& p : node->params) {
        SemType pt = SemType::fromTypeNode(p.type);
        int off = offset_;
        env_.declare(p.name, VarEntry{pt, off});
        offset_ -= 8;
        if (i < 6) out_ << "    movq " << argRegs[i] << ", " << off << "(%rbp)\n";
        ++i;
    }

    node->body->accept(this);

    out_ << ".end_" << node->name << ":\n";
    out_ << "    leave\n";
    out_ << "    ret\n";

    env_.exitScope();
}

void CodeGenerator::visit(GlobalVarDecl* /*node*/)    { /* TODO */ }
void CodeGenerator::visit(StructDecl* /*node*/)       { /* TODO */ }
void CodeGenerator::visit(TemplateFuncDecl* /*node*/) { /* TODO */ }

// ═════════════════════════════════════════════════════════════════════════════
// Sentencias
// ═════════════════════════════════════════════════════════════════════════════

void CodeGenerator::visit(Block* node) {
    env_.enterScope();
    for (auto s : node->stmts) s->accept(this);
    env_.exitScope();
}

void CodeGenerator::visit(ExprStmt* node) {
    node->expr->accept(this);
}

void CodeGenerator::visit(ReturnStmt* node) {
    if (node->expr) node->expr->accept(this);  // resultado en %rax
    out_ << "    jmp .end_" << current_func_ << "\n";
}

void CodeGenerator::visit(VarDeclStmt* node) {
    int off = offset_;
    offset_ -= 8;

    if (node->init) {
        node->init->accept(this);  // valor → %rax/%xmm0, tipo → cur_type_
        // 'auto' toma el tipo del inicializador (lo resolvió el semántico).
        SemType t = node->type->is_auto ? cur_type_
                                        : SemType::fromTypeNode(node->type);
        env_.declare(node->name, VarEntry{t, off});
        emitStore(t, off);
    } else {
        SemType t = SemType::fromTypeNode(node->type);
        env_.declare(node->name, VarEntry{t, off});
    }
    // TODO: arrays (node->dimensions / node->init_list)
}
void CodeGenerator::visit(IfStmt* /*node*/)       { /* TODO */ }
void CodeGenerator::visit(WhileStmt* /*node*/)    { /* TODO */ }
void CodeGenerator::visit(ForStmt* /*node*/)      { /* TODO */ }
void CodeGenerator::visit(ForRangeStmt* /*node*/) { /* TODO */ }
void CodeGenerator::visit(BreakStmt* /*node*/)    { /* TODO */ }
void CodeGenerator::visit(ContinueStmt* /*node*/) { /* TODO */ }
void CodeGenerator::visit(DeleteStmt* /*node*/)   { /* TODO */ }

// ═════════════════════════════════════════════════════════════════════════════
// Expresiones
// ═════════════════════════════════════════════════════════════════════════════

// ── Hojas (literales) ────────────────────────────────────────────────────────
void CodeGenerator::visit(IntLitExpr* node) {
    out_ << "    movq $" << node->value << ", %rax\n";
    cur_type_ = SemType{"int"};
}

void CodeGenerator::visit(BoolLitExpr* node) {
    out_ << "    movq $" << (node->value ? 1 : 0) << ", %rax\n";
    cur_type_ = SemType{"bool"};
}

void CodeGenerator::visit(CharLitExpr* node) {
    out_ << "    movq $" << charToCode(node->value) << ", %rax\n";
    cur_type_ = SemType{"char"};
}

void CodeGenerator::visit(FloatLitExpr* node) {
    // Los floats no caben como inmediato: van en .rodata y se cargan a %xmm0.
    std::string lbl = floatLabel(node->value);
    out_ << "    movsd " << lbl << "(%rip), %xmm0\n";
    cur_type_ = SemType{"float"};
}

void CodeGenerator::visit(StringLitExpr* node) {
    // Puntero al literal en .rodata → %rax.
    std::string lbl = strLabel(node->value);
    out_ << "    leaq " << lbl << "(%rip), %rax\n";
    cur_type_ = SemType{"string"};
}

// ── print / println (built-ins) ──────────────────────────────────────────────
// Emite una llamada a printf por argumento, eligiendo formato y registro según
// el tipo real de cada argumento (cur_type_).
void CodeGenerator::visit(CallExpr* node) {
    if (auto* id = dynamic_cast<IdExpr*>(node->callee)) {
        if (id->name == "print" || id->name == "println") {
            bool newline = (id->name == "println");
            for (auto arg : node->args) {
                arg->accept(this);  // valor → %rax (o %xmm0 si float); tipo → cur_type_

                if (cur_type_.base == "float" && !cur_type_.hasPointer()) {
                    // El valor ya está en %xmm0; %al = nº de regs XMM usados.
                    out_ << "    leaq __fmt_float(%rip), %rdi\n";
                    out_ << "    movl $1, %eax\n";
                } else {
                    out_ << "    movq %rax, %rsi\n";
                    const char* fmt = "__fmt_int";
                    if      (cur_type_.base == "char")   fmt = "__fmt_char";
                    else if (cur_type_.base == "string") fmt = "__fmt_str";
                    out_ << "    leaq " << fmt << "(%rip), %rdi\n";
                    out_ << "    movl $0, %eax\n";
                }
                out_ << "    call printf@PLT\n";
            }
            if (newline) {
                out_ << "    leaq __fmt_nl(%rip), %rdi\n";
                out_ << "    movl $0, %eax\n";
                out_ << "    call printf@PLT\n";
            }
            return;
        }
    }
    // TODO: llamada a función de usuario
}

void CodeGenerator::visit(IdExpr* node) {
    VarEntry* e = env_.lookup(node->name);
    if (!e) return;  // el semántico ya garantizó que existe
    emitLoad(e->type, e->offset);
    cur_type_ = e->type;
}

// Por ahora solo '=' a variable simple (IdExpr).
void CodeGenerator::visit(AssignExpr* node) {
    if (node->op == AssignOp::Assign) {
        if (auto* id = dynamic_cast<IdExpr*>(node->left)) {
            VarEntry* e = env_.lookup(id->name);
            node->right->accept(this);  // valor → %rax/%xmm0
            SemType t = e ? e->type : cur_type_;
            if (e) emitStore(t, e->offset);
            cur_type_ = t;  // el resultado de la asignación es el valor asignado
            return;
        }
    }
    // TODO: ops compuestos (+=, -=, ...), lvalues IndexExpr/MemberExpr/Deref
}

// ── Resto de expresiones (pendientes) ────────────────────────────────────────
void CodeGenerator::visit(BinaryExpr* /*node*/)    { /* TODO */ }
void CodeGenerator::visit(UnaryExpr* /*node*/)     { /* TODO */ }
void CodeGenerator::visit(CastExpr* /*node*/)      { /* TODO */ }
void CodeGenerator::visit(NewArrayExpr* /*node*/)  { /* TODO */ }
void CodeGenerator::visit(NewObjectExpr* /*node*/) { /* TODO */ }
void CodeGenerator::visit(IndexExpr* /*node*/)     { /* TODO */ }
void CodeGenerator::visit(MemberExpr* /*node*/)    { /* TODO */ }
void CodeGenerator::visit(PostfixExpr* /*node*/)   { /* TODO */ }
void CodeGenerator::visit(LambdaExpr* /*node*/)    { /* TODO */ }
