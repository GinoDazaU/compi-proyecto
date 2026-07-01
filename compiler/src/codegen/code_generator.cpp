#include "code_generator.h"
#include <algorithm>
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

// ─── Helper local: instrucción set<cc> para cada operador de comparación ──────
// floatCmp=true usa los códigos sin signo de ucomisd (setb/seta/...); =false los
// con signo de cmpq (setl/setg/...). Eq/Neq son iguales en ambos.
static const char* setccFor(BinaryOp op, bool floatCmp) {
    switch (op) {
        case BinaryOp::Lt:  return floatCmp ? "setb"  : "setl";
        case BinaryOp::Leq: return floatCmp ? "setbe" : "setle";
        case BinaryOp::Gt:  return floatCmp ? "seta"  : "setg";
        case BinaryOp::Geq: return floatCmp ? "setae" : "setge";
        case BinaryOp::Eq:  return "sete";
        case BinaryOp::Neq: return "setne";
        default:            return "sete";
    }
}

// ─── Registros de argumento de la convención System V AMD64 ──────────────────
// Bancos separados, con orden fijo por ABI: hasta 6 enteros/punteros y 8 floats.
static const char* const INT_ARG_REGS[]   = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static const char* const FLOAT_ARG_REGS[] = {"%xmm0","%xmm1","%xmm2","%xmm3",
                                             "%xmm4","%xmm5","%xmm6","%xmm7"};

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
    // 1ª sub-pasada: layouts de struct (los frames los necesitan para dimensionar
    // variables struct, así que deben existir antes de frameSize).
    for (auto d : program->decls)
        if (auto* s = dynamic_cast<StructDecl*>(d)) buildStructInfo(s);

    // 2ª sub-pasada: tamaño de frame y tipo de retorno de cada función.
    for (auto d : program->decls) {
        if (auto* f = dynamic_cast<FuncDecl*>(d)) {
            frame_sizes_[f->name] = frameSize(f);
            func_rets_[f->name]   = SemType::fromTypeNode(f->return_type);
            std::vector<SemType> ptypes;
            for (auto& p : f->params) ptypes.push_back(SemType::fromTypeNode(p.type));
            func_params_[f->name] = std::move(ptypes);
        }
    }
}

// Cuenta variables locales (8 bytes c/u por ahora) + parámetros, redondea a 16.
int CodeGenerator::frameSize(FuncDecl* f) {
    int slots = static_cast<int>(f->params.size());

    std::function<void(Stmt*)> countStmt = [&](Stmt* s) {
        if (!s) return;
        if (auto* vd = dynamic_cast<VarDeclStmt*>(s)) {
            slots += declSlots(vd);               // array→n, struct→size/8, escalar→1
        } else if (auto* b = dynamic_cast<Block*>(s)) {
            for (auto inner : b->stmts) countStmt(inner);
        } else if (auto* i = dynamic_cast<IfStmt*>(s)) {
            countStmt(i->then_branch);
            countStmt(i->else_branch);
        } else if (auto* w = dynamic_cast<WhileStmt*>(s)) {
            countStmt(w->body);
        } else if (auto* fr = dynamic_cast<ForStmt*>(s)) {
            if (fr->init.decl) slots += declSlots(fr->init.decl);
            countStmt(fr->body);
        }
    };
    countStmt(f->body);

    int bytes = slots * 8;
    if (bytes % 16 != 0) bytes += 16 - (bytes % 16);
    return bytes;
}

// Nº de elementos de una declaración: producto de sus dimensiones (1 si escalar).
// Las dimensiones de un array estático son constantes; se espera IntLitExpr.
int CodeGenerator::arrayElemCount(VarDeclStmt* node) {
    if (node->dimensions.empty()) return 1;
    long long total = 1;
    for (auto* dim : node->dimensions)
        if (auto* lit = dynamic_cast<IntLitExpr*>(dim)) total *= lit->value;
    return static_cast<int>(total);
}

// Slots de 8 bytes que reserva una declaración: array→nº elementos,
// struct (no puntero)→size/8, escalar→1.
int CodeGenerator::declSlots(VarDeclStmt* node) {
    if (!node->dimensions.empty()) return arrayElemCount(node);
    SemType t = SemType::fromTypeNode(node->type);
    if (!t.hasPointer() && structs_.count(t.base))
        return structs_[t.base].size / 8;
    return 1;
}

// Layout de un struct: cada miembro ocupa un slot de 8 bytes (igual que las
// variables locales), en orden de declaración. offsets[m] desde la base.
void CodeGenerator::buildStructInfo(StructDecl* s) {
    CodegenStructInfo info;
    int off = 0;
    for (auto& m : s->members) {
        info.offsets[m.name] = off;
        info.types[m.name]   = SemType::fromTypeNode(m.type);
        off += 8;
    }
    info.size = off;
    structs_[s->name] = info;
}

// ═════════════════════════════════════════════════════════════════════════════
// Helpers de emisión
// ═════════════════════════════════════════════════════════════════════════════

int CodeGenerator::nextLabel() { return label_counter_++; }

std::string CodeGenerator::label(const std::string& prefix, int n) {
    return "__" + prefix + "_" + std::to_string(n);
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
    std::string lbl = "__str_" + std::to_string(str_counter_++);
    string_literals_.push_back({lbl, lexeme});
    return lbl;
}

// Carga la variable en offset(%rbp) al registro de su tipo.
//   float → %xmm0 (movsd) ; bool/char → %al + zero-extend a %rax ; resto → %rax
void CodeGenerator::emitLoad(const SemType& t, int offset) {
    if (t.isFloat()) {
        out_ << "    movsd " << offset << "(%rbp), %xmm0\n";
    } else if (t.isByteSized()) {
        out_ << "    movb " << offset << "(%rbp), %al\n";
        out_ << "    movzbq %al, %rax\n";
    } else {
        out_ << "    movq " << offset << "(%rbp), %rax\n";
    }
}

// Guarda el resultado actual (en %rax o %xmm0) en offset(%rbp) según el tipo.
void CodeGenerator::emitStore(const SemType& t, int offset) {
    if (t.isFloat()) {
        out_ << "    movsd %xmm0, " << offset << "(%rbp)\n";
    } else if (t.isByteSized()) {
        out_ << "    movb %al, " << offset << "(%rbp)\n";
    } else {
        out_ << "    movq %rax, " << offset << "(%rbp)\n";
    }
}
// Carga el valor en (%rax) al registro de su tipo (sobrescribe %rax si es int).
void CodeGenerator::emitLoadIndirect(const SemType& t) {
    if (t.isFloat()) {
        out_ << "    movsd (%rax), %xmm0\n";
    } else if (t.isByteSized()) {
        out_ << "    movb (%rax), %al\n";
        out_ << "    movzbq %al, %rax\n";
    } else {
        out_ << "    movq (%rax), %rax\n";
    }
}

// Guarda el valor actual (%rax o %xmm0) en la dirección que hay en addrReg.
void CodeGenerator::emitStoreIndirect(const SemType& t, const std::string& addrReg) {
    if (t.isFloat()) {
        out_ << "    movsd %xmm0, (" << addrReg << ")\n";
    } else if (t.isByteSized()) {
        out_ << "    movb %al, (" << addrReg << ")\n";
    } else {
        out_ << "    movq %rax, (" << addrReg << ")\n";
    }
}

// Deja en %rax la dirección de un lvalue. cur_type_ ← tipo del valor allí.
void CodeGenerator::emitLvalueAddr(Expr* e) {
    cur_array_decay_ = false;
    if (auto* id = dynamic_cast<IdExpr*>(e)) {
        if (VarEntry* en = env_.lookup(id->name)) {
            out_ << "    leaq " << en->offset << "(%rbp), %rax\n";
            cur_type_ = en->type;
        }
        return;
    }
    if (auto* ix = dynamic_cast<IndexExpr*>(e)) {
        // Arrays estáticos (posiblemente multidim): almacenamiento plano row-major,
        // así que m[i][j] es aritmética de dirección, no una cadena de loads.
        // Aplanamos la cadena de índices para detectar la raíz.
        std::vector<Expr*> idxs;
        Expr* root = ix;
        while (auto* inner = dynamic_cast<IndexExpr*>(root)) {
            idxs.push_back(inner->index);
            root = inner->base;
        }
        std::reverse(idxs.begin(), idxs.end());

        IdExpr* rootId = dynamic_cast<IdExpr*>(root);
        VarEntry* en = rootId ? env_.lookup(rootId->name) : nullptr;
        if (en && en->is_array && idxs.size() <= en->dims.size()) {
            int              baseOff = en->offset;
            SemType          baseTy  = en->type;
            std::vector<int> dims    = en->dims;
            size_t n = dims.size(), k = idxs.size();

            // Índice lineal por Horner: acc = ((i0*d1 + i1)*d2 + i2)...
            idxs[0]->accept(this);                       // i0 → %rax
            for (size_t p = 1; p < k; ++p) {
                out_ << "    imulq $" << dims[p] << ", %rax\n";
                out_ << "    pushq %rax\n";
                idxs[p]->accept(this);                   // ip → %rax
                out_ << "    popq %rcx\n";
                out_ << "    addq %rcx, %rax\n";
            }
            // Índice parcial (k<n): escala por el tamaño del sub-array restante.
            int tail = 1;
            for (size_t p = k; p < n; ++p) tail *= dims[p];
            out_ << "    imulq $" << tail * 8 << ", %rax\n";  // índice → offset en bytes
            out_ << "    leaq " << baseOff << "(%rbp), %rcx\n";
            out_ << "    addq %rcx, %rax\n";              // dirección del (sub)elemento

            for (size_t p = 0; p < k; ++p) baseTy = baseTy.deref();
            cur_type_        = baseTy;
            cur_array_decay_ = (k < n);                   // sub-array: la dirección es el valor
            return;
        }

        // Puntero o string: un solo nivel (se carga el puntero base y se indexa).
        // string: char empaquetado (stride 1). Punteros: slots de 8 bytes.
        ix->base->accept(this);          // puntero base → %rax
        SemType bt = cur_type_;
        bool isStr = (bt.base == "string");
        out_ << "    pushq %rax\n";
        ix->index->accept(this);         // índice → %rax
        out_ << "    movq %rax, %rcx\n";
        out_ << "    popq %rax\n";
        if (!isStr) out_ << "    imulq $8, %rcx\n";
        out_ << "    addq %rcx, %rax\n";  // dirección del elemento
        cur_type_ = isStr ? SemType{"char"} : bt.deref();
        return;
    }
    if (auto* u = dynamic_cast<UnaryExpr*>(e)) {
        if (u->op == UnaryOp::Deref) {
            // *p como lvalue: el valor del puntero ES la dirección destino.
            u->expr->accept(this);     // puntero → %rax
            cur_type_ = cur_type_.deref();
            return;
        }
    }
    if (auto* mem = dynamic_cast<MemberExpr*>(e)) {
        // s.x  → dirección del struct + offset del miembro
        // p->x → valor del puntero (= dirección del struct) + offset del miembro
        SemType st;
        if (mem->is_arrow) {
            mem->base->accept(this);       // puntero (dirección del struct) → %rax
            st = cur_type_.deref();
        } else {
            emitLvalueAddr(mem->base);     // dirección del struct → %rax
            st = cur_type_;
        }
        const CodegenStructInfo& info = structs_.at(st.base);
        int moff = info.offsets.at(mem->member);
        if (moff) out_ << "    addq $" << moff << ", %rax\n";
        cur_type_ = info.types.at(mem->member);
        return;
    }
}

// Promoción implícita al asignar a un destino float: si el valor recién
// evaluado es entero (int/bool/char en %rax) y el destino es float, lo convierte
// a %xmm0 con cvtsi2sdq y actualiza cur_type_. No-op en cualquier otro caso.
void CodeGenerator::emitPromote(const SemType& target) {
    if (target.isFloat() && !cur_type_.isFloat() && !cur_type_.hasPointer()) {
        out_ << "    cvtsi2sdq %rax, %xmm0\n";
        cur_type_ = SemType{"float"};
    }
}

void CodeGenerator::emitPush(const SemType& t) {
    if (t.isFloat()) {
        out_ << "    subq $8, %rsp\n";
        out_ << "    movsd %xmm0, (%rsp)\n";
    } else {
        out_ << "    pushq %rax\n";
    }
}

void CodeGenerator::emitPop(const SemType& t, const std::string& reg) {
    if (t.isFloat()) {
        out_ << "    movsd (%rsp), " << reg << "\n";
        out_ << "    addq $8, %rsp\n";
    } else {
        out_ << "    popq " << reg << "\n";
    }
}

void CodeGenerator::emitCompareZero(const SemType& t) {
    if (t.isFloat()) {
        out_ << "    xorpd %xmm1, %xmm1\n";
        out_ << "    ucomisd %xmm1, %xmm0\n";  // %xmm0 == 0 → ZF
    } else {
        out_ << "    cmpq $0, %rax\n";
    }
}

void CodeGenerator::emitToBool(const SemType& t) {
    emitCompareZero(t);
    out_ << "    movl $0, %eax\n";
    out_ << "    setne %al\n";        // %rax = (valor != 0)
    out_ << "    movzbq %al, %rax\n";
}

// Tras una comparación ya emitida (cmpq/ucomisd), materializa el booleano 0/1
// en %rax con el set<cc> del operador. cur_type_ ← bool.
void CodeGenerator::emitSetccBool(BinaryOp op, bool floatCmp) {
    out_ << "    movl $0, %eax\n";
    out_ << "    " << setccFor(op, floatCmp) << " %al\n";
    out_ << "    movzbq %al, %rax\n";
    cur_type_ = SemType{"bool"};
}

void CodeGenerator::emitCondJumpIfFalse(Expr* cond, const std::string& label) {
    cond->accept(this);  // valor → %rax (o %xmm0 si float)
    emitCompareZero(cur_type_);
    out_ << "    je " << label << "\n";
}

void CodeGenerator::emitDataSection() {
    out_ << ".data\n";
    out_ << "__fmt_int:   .string \"%ld\"\n";
    out_ << "__fmt_float: .string \"%lf\"\n";
    out_ << "__fmt_char:  .string \"%c\"\n";
    out_ << "__fmt_bool:  .string \"%d\"\n";
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

    // Guardar parámetros: enteros/ptr desde %rdi…, floats desde %xmm0…
    // (System V usa bancos de registros separados, con índice propio cada uno).
    int nInt = 0, nFloat = 0;
    for (auto& p : node->params) {
        SemType pt = SemType::fromTypeNode(p.type);
        int off = offset_;
        env_.declare(p.name, VarEntry{pt, off});
        offset_ -= 8;
        if (pt.isFloat()) {
            if (nFloat < 8) out_ << "    movsd " << FLOAT_ARG_REGS[nFloat] << ", " << off << "(%rbp)\n";
            ++nFloat;
        } else {
            if (nInt < 6) out_ << "    movq " << INT_ARG_REGS[nInt] << ", " << off << "(%rbp)\n";
            ++nInt;
        }
    }

    node->body->accept(this);

    out_ << ".end_" << node->name << ":\n";
    out_ << "    leave\n";
    out_ << "    ret\n";

    env_.exitScope();
}

// El layout (offsets, size) se calcula en firstPass; aquí no se emite código.
void CodeGenerator::visit(StructDecl* /*node*/) {}

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
    if (node->expr) {
        node->expr->accept(this);  // resultado en %rax/%xmm0
        emitPromote(func_rets_[current_func_]);  // int→float si la func retorna float
    }
    out_ << "    jmp .end_" << current_func_ << "\n";
}

void CodeGenerator::visit(VarDeclStmt* node) {
    // ── Array estático: reserva n slots inline; la var decae a puntero ────────
    if (!node->dimensions.empty()) {
        int count = arrayElemCount(node);
        int base  = offset_ - (count - 1) * 8;  // slot más bajo = arr[0]
        offset_  -= count * 8;

        SemType elem = SemType::fromTypeNode(node->type);   // tipo del elemento
        SemType ptr  = elem;
        std::vector<int> dims;
        for (auto* d : node->dimensions) {
            ptr.mods.push_back(PtrMod::Pointer);            // decae a T* (T** si 2D)
            auto* lit = dynamic_cast<IntLitExpr*>(d);       // dimensiones constantes
            dims.push_back(lit ? static_cast<int>(lit->value) : 0);
        }
        env_.declare(node->name, VarEntry{ptr, base, /*is_array=*/true, dims});

        // init_list: arr[i] = init_list[i]
        for (size_t i = 0; i < node->init_list.size(); ++i) {
            node->init_list[i]->accept(this);               // valor → %rax/%xmm0
            emitPromote(elem);                              // int→float si aplica
            emitStore(elem, base + static_cast<int>(i) * 8);
        }
        return;
    }

    // ── Variable struct: reserva size bytes; la var decae a su dirección base ──
    {
        SemType t = SemType::fromTypeNode(node->type);
        if (!t.hasPointer() && structs_.count(t.base)) {
            int size = structs_[t.base].size;
            int base = offset_ - (size - 8);  // slot más bajo = miembro en offset 0
            offset_ -= size;
            env_.declare(node->name, VarEntry{t, base, /*is_array=*/true});
            return;  // sin inicializador (la gramática no tiene literales de struct)
        }
    }

    // ── Variable escalar ──────────────────────────────────────────────────────
    int off = offset_;
    offset_ -= 8;

    if (node->init) {
        node->init->accept(this);  // valor → %rax/%xmm0, tipo → cur_type_
        // 'auto' toma el tipo del inicializador (lo resolvió el semántico).
        SemType t = node->type->is_auto ? cur_type_
                                        : SemType::fromTypeNode(node->type);
        emitPromote(t);  // int→float si el destino es float (no-op con auto)
        env_.declare(node->name, VarEntry{t, off});
        emitStore(t, off);
    } else {
        SemType t = SemType::fromTypeNode(node->type);
        env_.declare(node->name, VarEntry{t, off});
    }
}
void CodeGenerator::visit(IfStmt* node) {
    int n = nextLabel();
    std::string endLabel = label("endif", n);

    if (node->else_branch) {
        std::string elseLabel = label("else", n);
        emitCondJumpIfFalse(node->condition, elseLabel);
        node->then_branch->accept(this);
        out_ << "    jmp " << endLabel << "\n";
        out_ << elseLabel << ":\n";
        node->else_branch->accept(this);  // Block o IfStmt (else if)
    } else {
        emitCondJumpIfFalse(node->condition, endLabel);
        node->then_branch->accept(this);
    }
    out_ << endLabel << ":\n";
}
void CodeGenerator::visit(WhileStmt* node) {
    int n = nextLabel();
    std::string startLabel = label("while",    n);
    std::string endLabel   = label("endwhile", n);

    // {target de continue, target de break}
    loop_labels_.push({startLabel, endLabel});

    out_ << startLabel << ":\n";
    emitCondJumpIfFalse(node->condition, endLabel);
    node->body->accept(this);
    out_ << "    jmp " << startLabel << "\n";
    out_ << endLabel << ":\n";

    loop_labels_.pop();
}
void CodeGenerator::visit(ForStmt* node) {
    int n = nextLabel();
    std::string condLabel = label("for",    n);
    std::string updLabel  = label("forupd", n);
    std::string endLabel  = label("endfor", n);

    env_.enterScope();  // la variable del init vive solo dentro del for

    if (node->init.decl)      node->init.decl->accept(this);
    else if (node->init.expr) node->init.expr->accept(this);

    out_ << condLabel << ":\n";
    if (node->condition)  // sin condición → loop infinito (sale por break/return)
        emitCondJumpIfFalse(node->condition, endLabel);

    // continue salta al update (no se lo salta); break al final
    loop_labels_.push({updLabel, endLabel});
    node->body->accept(this);
    loop_labels_.pop();

    out_ << updLabel << ":\n";
    if (node->update) node->update->accept(this);
    out_ << "    jmp " << condLabel << "\n";
    out_ << endLabel << ":\n";

    env_.exitScope();
}

void CodeGenerator::visit(BreakStmt* /*node*/) {
    if (loop_labels_.empty()) return;  // el semántico ya garantiza estar en un loop
    out_ << "    jmp " << loop_labels_.top().second << "\n";
}
void CodeGenerator::visit(ContinueStmt* /*node*/) {
    if (loop_labels_.empty()) return;
    out_ << "    jmp " << loop_labels_.top().first << "\n";
}
// delete p / delete[] p: free(p). El '[]' no cambia nada (un solo bloque).
void CodeGenerator::visit(DeleteStmt* node) {
    node->expr->accept(this);         // puntero → %rax
    out_ << "    movq %rax, %rdi\n";
    out_ << "    call free@PLT\n";
}

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

// Despacha según el callee: built-in print/println o función de usuario.
void CodeGenerator::visit(CallExpr* node) {
    if (auto* id = dynamic_cast<IdExpr*>(node->callee)) {
        if (id->name == "print" || id->name == "println") {
            emitBuiltinPrint(node, id->name == "println");
            return;
        }
        if (frame_sizes_.count(id->name)) {
            emitUserCall(node, id->name);
            return;
        }
    }
}

// ── print / println (built-ins) ──────────────────────────────────────────────
// Emite una llamada a printf por argumento, eligiendo formato y registro según
// el tipo real de cada argumento (cur_type_).
void CodeGenerator::emitBuiltinPrint(CallExpr* node, bool newline) {
    for (auto arg : node->args) {
        arg->accept(this);  // valor → %rax (o %xmm0 si float); tipo → cur_type_

        if (cur_type_.isFloat()) {
            // El valor ya está en %xmm0; %al = nº de regs XMM usados.
            out_ << "    leaq __fmt_float(%rip), %rdi\n";
            out_ << "    movl $1, %eax\n";
        } else {
            out_ << "    movq %rax, %rsi\n";
            const char* fmt = "__fmt_int";
            if      (cur_type_.base == "char")   fmt = "__fmt_char";
            else if (cur_type_.base == "bool")   fmt = "__fmt_bool";
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
}

// ── Llamada a función de usuario ──────────────────────────────────────────────
// Convención System V (codegen.md §8): evaluar args en orden y apilarlos, luego
// sacarlos en orden inverso a los registros de su banco (int en %rdi…/%r9, float
// en %xmm0…%xmm7). Resultado en %rax (o %xmm0 si float).
void CodeGenerator::emitUserCall(CallExpr* node, const std::string& name) {
    size_t n = node->args.size();

    // 1. Evaluar y apilar cada arg; recordar su banco y su índice de registro.
    const std::vector<SemType>& ptypes = func_params_[name];

    std::vector<bool> isFloat(n);
    std::vector<int>  regIdx(n);
    int nInt = 0, nFloat = 0;
    for (size_t i = 0; i < n; ++i) {
        node->args[i]->accept(this);   // valor → %rax o %xmm0; tipo → cur_type_
        // Promoción al tipo del parámetro (int→float): debe ocurrir antes de
        // elegir el banco, para que un int pasado a un param float viaje por %xmm.
        if (i < ptypes.size()) emitPromote(ptypes[i]);
        bool f = (cur_type_.isFloat());
        isFloat[i] = f;
        regIdx[i]  = f ? nFloat++ : nInt++;
        emitPush(cur_type_);
    }

    // 2. Sacar de la pila en orden inverso (la cima es el último arg).
    for (size_t k = n; k-- > 0; ) {
        if (isFloat[k]) {
            if (regIdx[k] < 8) emitPop(SemType{"float"}, FLOAT_ARG_REGS[regIdx[k]]);
        } else {
            if (regIdx[k] < 6) out_ << "    popq " << INT_ARG_REGS[regIdx[k]] << "\n";
        }
    }

    out_ << "    call " << name << "\n";

    auto it = func_rets_.find(name);
    cur_type_ = (it != func_rets_.end()) ? it->second : SemType{"int"};
}

void CodeGenerator::visit(IdExpr* node) {
    VarEntry* e = env_.lookup(node->name);
    if (!e) return;  // el semántico ya garantizó que existe
    if (e->is_array) {
        // Un array decae a puntero: su valor es la dirección de arr[0].
        out_ << "    leaq " << e->offset << "(%rbp), %rax\n";
    } else {
        emitLoad(e->type, e->offset);
    }
    cur_type_ = e->type;
}

void CodeGenerator::visit(AssignExpr* node) {
    // Variable simple: store directo por offset.
    if (auto* id = dynamic_cast<IdExpr*>(node->left)) {
        VarEntry* e = env_.lookup(id->name);
        node->right->accept(this);  // valor → %rax/%xmm0
        SemType t = e ? e->type : cur_type_;
        if (e) { emitPromote(t); emitStore(t, e->offset); }  // int→float si aplica
        cur_type_ = t;  // el resultado de la asignación es el valor asignado
        return;
    }

    // lvalue por dirección (arr[i], *p; luego s.x): calcular dirección, evaluar
    // el RHS, y guardar de forma indirecta.
    emitLvalueAddr(node->left);  // dirección → %rax
    SemType t = cur_type_;
    out_ << "    pushq %rax\n";   // guardar dirección durante el RHS
    node->right->accept(this);    // valor → %rax/%xmm0
    emitPromote(t);               // int→float si el destino es float
    out_ << "    popq %rcx\n";     // dirección → %rcx
    emitStoreIndirect(t, "%rcx");
    cur_type_ = t;
}

void CodeGenerator::visit(BinaryExpr* node) {
    // Operadores lógicos: evaluación con cortocircuito, igual que C++ real.
    // Se manejan aparte (antes de evaluar ambos lados) porque el lado derecho
    // NO debe evaluarse si el izquierdo ya determina el resultado. Esto importa
    // para idiomas con punteros como `p != nullptr && p->x`.
    if (node->op == BinaryOp::And || node->op == BinaryOp::Or) {
        bool isAnd = (node->op == BinaryOp::And);
        int  n     = nextLabel();
        std::string shortLabel = label("logic_short", n);
        std::string endLabel   = label("logic_end",   n);

        // El resultado se normaliza a 0/1 (así `2 && 1` da 1, no un AND bit a
        // bit). Para el corto solo importa si el izquierdo es 0 o no, sin
        // normalizarlo: el valor 0/1 del izquierdo no se usa.
        node->left->accept(this);
        emitCompareZero(cur_type_);        // flags: ¿left == 0?
        if (isAnd) out_ << "    je "  << shortLabel << "\n";  // &&: left falso → corto en 0
        else       out_ << "    jne " << shortLabel << "\n";  // ||: left verdad → corto en 1

        node->right->accept(this);         // solo se evalúa si no hubo cortocircuito
        emitToBool(cur_type_);             // %rax = (right != 0) → resultado final
        out_ << "    jmp " << endLabel << "\n";

        out_ << shortLabel << ":\n";
        out_ << "    movq $" << (isAnd ? 0 : 1) << ", %rax\n";

        out_ << endLabel << ":\n";
        cur_type_ = SemType{"bool"};
        return;
    }

    // Evaluamos al left primero en todos los casos
    node->left->accept(this);
    SemType leftType = cur_type_;

    // Guardar resultado de left en la pila
    emitPush(leftType);

    node->right->accept(this);
    SemType rightType = cur_type_;

    // El TypeChecker permite mezclar int y float, pero no modifica el AST.
    // Como los operandos conservan su tipo original, hacemos la conversión
    // a float (cvtsi2sdq) aquí en el codegen si es necesario.

    bool leftIsFloat  = leftType.isFloat();
    bool rightIsFloat = rightType.isFloat();
    bool useFloatPath = leftIsFloat || rightIsFloat;

    if (useFloatPath) {
        // Ruta float (ambos o mixto)
        if (rightIsFloat) {
            out_ << "    movsd %xmm0, %xmm1\n";
        } else {
            out_ << "    cvtsi2sdq %rax, %xmm1\n";
        }

        if (leftIsFloat) {
            emitPop(leftType, "%xmm0");
        } else {
            emitPop(leftType, "%rax");
            out_ << "    cvtsi2sdq %rax, %xmm0\n";
        }

        switch (node->op) {
            case BinaryOp::Add: out_ << "    addsd %xmm1, %xmm0\n"; break;
            case BinaryOp::Sub: out_ << "    subsd %xmm1, %xmm0\n"; break;
            case BinaryOp::Mul: out_ << "    mulsd %xmm1, %xmm0\n"; break;
            case BinaryOp::Div: out_ << "    divsd %xmm1, %xmm0\n"; break;

            case BinaryOp::Lt:  case BinaryOp::Leq:
            case BinaryOp::Gt:  case BinaryOp::Geq:
            case BinaryOp::Eq:  case BinaryOp::Neq:
                out_ << "    ucomisd %xmm1, %xmm0\n";
                emitSetccBool(node->op, /*floatCmp=*/true);
                return;

            default: break;
        }
        cur_type_ = SemType{"float"};
        return;
    }

    // Ruta entera (ambos int)
    out_ << "    movq %rax, %rcx\n";
    emitPop(leftType, "%rax");

    switch (node->op) {
        // Aritmeticos
        case BinaryOp::Add: out_ << "    addq %rcx, %rax\n"; break;
        case BinaryOp::Sub: out_ << "    subq %rcx, %rax\n"; break;
        case BinaryOp::Mul: out_ << "    imulq %rcx, %rax\n"; break;
        case BinaryOp::Div:
            out_ << "    cqto\n";
            out_ << "    idivq %rcx\n";
            break;
        case BinaryOp::Mod:
            out_ << "    cqto\n";
            out_ << "    idivq %rcx\n";
            out_ << "    movq %rdx, %rax\n";
            break;

        // Comparaciones
        case BinaryOp::Lt:  case BinaryOp::Leq:
        case BinaryOp::Gt:  case BinaryOp::Geq:
        case BinaryOp::Eq:  case BinaryOp::Neq:
            out_ << "    cmpq %rcx, %rax\n";
            emitSetccBool(node->op, /*floatCmp=*/false);
            return;

        // && y || se resuelven arriba con cortocircuito; inalcanzables aquí.
        case BinaryOp::And:
        case BinaryOp::Or:
            break;
    }
    // Promoción numérica: el resultado es el tipo de mayor rango (char + int →
    // int). La aritmética de punteros conserva el puntero; se excluye antes de
    // promote() porque éste ignora 'mods'.
    if (leftType.hasPointer())       cur_type_ = leftType;
    else if (rightType.hasPointer()) cur_type_ = rightType;
    else                             cur_type_ = SemType::promote(leftType, rightType);
}

// ++/-- sobre cualquier lvalue. Calcula su dirección una sola vez, carga el
// valor, le suma/resta 1 (entero o float) y lo guarda. El resultado en el
// registro es el viejo (postfix) o el nuevo (prefix).
void CodeGenerator::emitIncDec(Expr* lvalue, bool inc, bool postfix) {
    emitLvalueAddr(lvalue);             // dirección → %rax; cur_type_ = tipo
    SemType t = cur_type_;
    out_ << "    movq %rax, %rcx\n";    // %rcx = dirección (se conserva)

    if (t.isFloat()) {
        out_ << "    movsd (%rcx), %xmm0\n";                       // valor actual
        out_ << "    movsd " << floatLabel(1.0) << "(%rip), %xmm1\n";
        out_ << "    movsd %xmm0, %xmm2\n";                        // copia del viejo
        out_ << (inc ? "    addsd %xmm1, %xmm2\n" : "    subsd %xmm1, %xmm2\n");
        out_ << "    movsd %xmm2, (%rcx)\n";                       // guardar nuevo
        if (!postfix) out_ << "    movsd %xmm2, %xmm0\n";          // prefix → nuevo
    } else {
        bool byte = t.isByteSized();
        if (byte) out_ << "    movb (%rcx), %al\n    movzbq %al, %rax\n";
        else      out_ << "    movq (%rcx), %rax\n";              // valor actual (viejo)
        out_ << (inc ? "    leaq 1(%rax), %rdx\n" : "    leaq -1(%rax), %rdx\n");
        if (byte) out_ << "    movb %dl, (%rcx)\n";
        else      out_ << "    movq %rdx, (%rcx)\n";              // guardar nuevo
        if (!postfix) out_ << "    movq %rdx, %rax\n";            // prefix → nuevo
    }
    cur_type_ = t;
}

void CodeGenerator::visit(UnaryExpr* node) {
    switch (node->op) {
        case UnaryOp::Neg: {
            node->expr->accept(this);
            if (cur_type_.isFloat()) {
                out_ << "    movsd %xmm0, %xmm1\n";
                out_ << "    xorpd %xmm0, %xmm0\n";
                out_ << "    subsd %xmm1, %xmm0\n";
            } else {
                out_ << "    negq %rax\n";
                cur_type_ = SemType{"int"};  // -char/-bool promueven a int
            }
            break;
        }
        case UnaryOp::Not: {
            node->expr->accept(this);
            // !x = (x == 0). Para float se compara contra 0.0 con ucomisd.
            emitCompareZero(cur_type_);
            out_ << "    movl $0, %eax\n";
            out_ << "    sete %al\n";
            out_ << "    movzbq %al, %rax\n";
            cur_type_ = SemType{"bool"};
            break;
        }
        case UnaryOp::PreInc:
            emitIncDec(node->expr, /*inc=*/true,  /*postfix=*/false);
            break;
        case UnaryOp::PreDec:
            emitIncDec(node->expr, /*inc=*/false, /*postfix=*/false);
            break;
        case UnaryOp::Deref: {
            // *p como rvalue: cargar el valor apuntado.
            node->expr->accept(this);     // puntero → %rax
            SemType t = cur_type_.deref();
            emitLoadIndirect(t);          // valor en (%rax) → registro del tipo
            cur_type_ = t;
            break;
        }
        case UnaryOp::AddrOf:
            // &lvalue: la dirección del lvalue es el resultado; el tipo es T*.
            emitLvalueAddr(node->expr);   // dirección → %rax; cur_type_ = T
            cur_type_.mods.push_back(PtrMod::Pointer);
            break;
    }
}

// new T[n]: malloc(n*8) (cada elemento ocupa un slot de 8 bytes). Devuelve T*.
void CodeGenerator::visit(NewArrayExpr* node) {
    node->size->accept(this);         // n → %rax
    out_ << "    imulq $8, %rax\n";
    out_ << "    movq %rax, %rdi\n";
    out_ << "    call malloc@PLT\n";  // puntero → %rax
    SemType t = SemType::fromTypeNode(node->type);
    t.mods.push_back(PtrMod::Pointer);
    cur_type_ = t;
}

// new T (struct): calloc(1, size) para dejar los campos en cero. Devuelve T*.
void CodeGenerator::visit(NewObjectExpr* node) {
    SemType t = SemType::fromTypeNode(node->type);
    int size = structs_.count(t.base) ? structs_[t.base].size : 8;
    out_ << "    movq $1, %rdi\n";
    out_ << "    movq $" << size << ", %rsi\n";
    out_ << "    call calloc@PLT\n"; // memoria en cero; puntero → %rax
    t.mods.push_back(PtrMod::Pointer);
    cur_type_ = t;
}

// arr[i] como rvalue: dirección del elemento → %rax, luego carga su valor.
void CodeGenerator::visit(IndexExpr* node) {
    emitLvalueAddr(node);     // dirección del elemento → %rax; cur_type_ = tipo elem
    SemType et = cur_type_;
    if (cur_array_decay_) { cur_type_ = et; return; }  // sub-array: dirección = valor
    emitLoadIndirect(et);
    cur_type_ = et;
}

// s.x / p->x como rvalue: dirección del miembro → %rax, luego carga su valor.
void CodeGenerator::visit(MemberExpr* node) {
    emitLvalueAddr(node);     // dirección del miembro → %rax; cur_type_ = tipo miembro
    SemType t = cur_type_;
    emitLoadIndirect(t);
    cur_type_ = t;
}
// base++ / base-- : igual que prefijo pero el resultado es el valor anterior.
void CodeGenerator::visit(PostfixExpr* node) {
    emitIncDec(node->base, /*inc=*/node->is_inc, /*postfix=*/true);
}
