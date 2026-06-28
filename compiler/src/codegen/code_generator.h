#pragma once
#include <ostream>
#include <string>
#include <stack>
#include <vector>
#include <utility>
#include <unordered_map>
#include "../parser/visitor.h"
#include "../parser/ast.h"
#include "../semantic/sem_type.h"
#include "../semantic/symbol_table.h"

// ─── Info de struct para codegen (incluye offsets) ───────────────────────────
struct CodegenStructInfo {
    std::unordered_map<std::string, int>     offsets; // miembro → offset desde base
    std::unordered_map<std::string, SemType> types;   // miembro → tipo
    int size = 0;
};

// ─── Entrada del environment: tipo + offset desde %rbp ───────────────────────
struct VarEntry {
    SemType type;
    int     offset   = 0;
    // true si la variable es un array estático reservado inline en el frame.
    // En ese caso 'offset' apunta a arr[0] y la variable decae a puntero: su
    // "valor" es esa dirección (leaq), no el contenido del slot.
    bool    is_array = false;
    // Tamaños de cada dimensión (solo arrays estáticos). El almacenamiento es
    // plano row-major, así que m[i][j] se direcciona con estos strides.
    std::vector<int> dims;
};

// ─── CodeGenerator ───────────────────────────────────────────────────────────
class CodeGenerator : public Visitor {
public:
    CodeGenerator(std::ostream& out);
    ~CodeGenerator() override = default;

    void gencode(Program* program);

private:
    std::ostream& out_;

    SymbolTable<VarEntry> env_;

    // Precalculados en firstPass
    std::unordered_map<std::string, int>                  frame_sizes_;  // func → bytes
    std::unordered_map<std::string, SemType>              func_rets_;    // func → tipo de retorno
    std::unordered_map<std::string, std::vector<SemType>> func_params_;  // func → tipos de params
    std::unordered_map<std::string, CodegenStructInfo> structs_;

    // Estado durante la emisión
    int         offset_        = -8;  // offset de la próxima var local
    int         label_counter_ = 0;   // contador global de labels únicos
    int         str_counter_   = 0;   // contador de string literals
    int         float_counter_ = 0;   // contador de float literals
    std::string current_func_;

    // Tipo de la última expresión evaluada (el "resultado" del visit actual).
    // Mecanismo equivalente a devolver el tipo desde accept(): cada visit de
    // expresión lo deja aquí, y el padre lo consulta (p.ej. print elige formato
    // y registro según esto, %rax vs %xmm0).
    SemType cur_type_;

    // true cuando emitLvalueAddr dejó en %rax la dirección de un (sub)array que
    // decae a puntero: el valor ES esa dirección, no se debe hacer load.
    bool cur_array_decay_ = false;

    // {label_inicio, label_fin} del loop actual (para break/continue)
    std::stack<std::pair<std::string, std::string>> loop_labels_;

    // Recolectores de constantes para la sección .rodata
    std::vector<std::pair<std::string, std::string>> string_literals_;
    std::vector<std::pair<std::string, double>>      float_literals_;

    // ─── Primera pasada: frame sizes y struct layouts ─────────────────────
    void firstPass(Program* program);
    int  frameSize(FuncDecl* f);                 // calcula y redondea a múltiplo de 16
    int  arrayElemCount(VarDeclStmt* node);      // nº de elementos (1 si no es array)
    int  declSlots(VarDeclStmt* node);           // slots de 8 bytes que reserva
    void buildStructInfo(StructDecl* s);

    // ─── Helpers de emisión ───────────────────────────────────────────────
    int         nextLabel();

    // Construye un label interno único: "__<prefix>_<n>".
    std::string label(const std::string& prefix, int n);

    // Registran (o reutilizan) una constante en .rodata y devuelven su label.
    std::string floatLabel(double v);
    std::string strLabel(const std::string& lexeme);

    // load/store según tipo (int→%rax, float→%xmm0, bool/char→%al+movzbq)
    void emitLoad (const SemType& t, int offset);
    void emitStore(const SemType& t, int offset);

    // Igual que emitLoad/emitStore pero con la dirección en un registro (indirecto).
    // Load lee de (%rax) al registro del tipo; store escribe a (addrReg).
    void emitLoadIndirect (const SemType& t);
    void emitStoreIndirect(const SemType& t, const std::string& addrReg);

    // Deja en %rax la DIRECCIÓN de un lvalue (IdExpr, IndexExpr; luego s.x, *p).
    // cur_type_ queda con el tipo del valor en esa dirección.
    void emitLvalueAddr(Expr* e);

    // Incremento/decremento sobre cualquier lvalue. postfix=true deja el valor
    // ANTERIOR como resultado (x++); postfix=false deja el nuevo (++x).
    void emitIncDec(Expr* lvalue, bool inc, bool postfix);

    // Convierte el valor recién evaluado a float (cvtsi2sdq) si el destino es
    // float y el valor es entero. Implementa la promoción implícita en asignaciones.
    void emitPromote(const SemType& target);

    // push/pop genérico para expresiones binarias
    void emitPush(const SemType& t);
    void emitPop (const SemType& t, const std::string& reg); // reg: %rax o %xmm0

    // Compara el valor recién evaluado (en %rax, o %xmm0 si float) contra 0,
    // dejando las flags listas para un salto condicional. Unifica la divergencia
    // float (xorpd+ucomisd) vs int (cmpq $0).
    void emitCompareZero(const SemType& t);
    // Normaliza ese valor a un booleano 0/1 en %rax (valor != 0).
    void emitToBool(const SemType& t);
    // Tras una comparación ya emitida, deja el booleano 0/1 del set<cc> en %rax.
    void emitSetccBool(BinaryOp op, bool floatCmp);

    // Evalúa una condición y salta a 'label' si es falsa (==0). Usado por if/while/for.
    void emitCondJumpIfFalse(Expr* cond, const std::string& label);

    // Sub-emisores de CallExpr (built-in print/println vs función de usuario).
    void emitBuiltinPrint(CallExpr* node, bool newline);
    void emitUserCall(CallExpr* node, const std::string& name);

    // sección .data al inicio
    void emitDataSection();
    // sección .rodata con float/string literals recolectados
    void emitRodataSection();

public:
    // ─── Expresiones ─────────────────────────────────────────────────────
    void visit(IntLitExpr* node)       override;
    void visit(FloatLitExpr* node)     override;
    void visit(BoolLitExpr* node)      override;
    void visit(CharLitExpr* node)      override;
    void visit(StringLitExpr* node)    override;
    void visit(IdExpr* node)           override;
    void visit(BinaryExpr* node)       override;
    void visit(UnaryExpr* node)        override;
    void visit(AssignExpr* node)       override;
    void visit(NewArrayExpr* node)     override;
    void visit(NewObjectExpr* node)    override;
    void visit(IndexExpr* node)        override;
    void visit(CallExpr* node)         override;
    void visit(MemberExpr* node)       override;
    void visit(PostfixExpr* node)      override;
    void visit(LambdaExpr* node)       override;

    // ─── Sentencias ───────────────────────────────────────────────────────
    void visit(Block* node)            override;
    void visit(VarDeclStmt* node)      override;
    void visit(ExprStmt* node)         override;
    void visit(IfStmt* node)           override;
    void visit(WhileStmt* node)        override;
    void visit(ForStmt* node)          override;
    void visit(ReturnStmt* node)       override;
    void visit(BreakStmt* node)        override;
    void visit(ContinueStmt* node)     override;
    void visit(DeleteStmt* node)       override;

    // ─── Declaraciones globales ───────────────────────────────────────────
    void visit(Program* node)          override;
    void visit(StructDecl* node)       override;
    void visit(FuncDecl* node)         override;
    void visit(TemplateFuncDecl* node) override;
};
