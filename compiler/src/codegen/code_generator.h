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
    int     offset = 0;
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
    std::unordered_map<std::string, int>               frame_sizes_; // func → bytes
    std::unordered_map<std::string, SemType>           func_rets_;   // func → tipo de retorno
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

    // {label_inicio, label_fin} del loop actual (para break/continue)
    std::stack<std::pair<std::string, std::string>> loop_labels_;

    // Recolectores de constantes para la sección .rodata
    std::vector<std::pair<std::string, std::string>> string_literals_;
    std::vector<std::pair<std::string, double>>      float_literals_;

    // ─── Primera pasada: frame sizes y struct layouts ─────────────────────
    void firstPass(Program* program);
    int  frameSize(FuncDecl* f);                 // calcula y redondea a múltiplo de 16
    void buildStructInfo(StructDecl* s);

    // ─── Helpers de emisión ───────────────────────────────────────────────
    int         nextLabel();
    std::string newStrLabel();

    // Registran (o reutilizan) una constante en .rodata y devuelven su label.
    std::string floatLabel(double v);
    std::string strLabel(const std::string& lexeme);

    // load/store según tipo (int→%rax, float→%xmm0, bool/char→%al+movzbq)
    void emitLoad (const SemType& t, int offset);
    void emitStore(const SemType& t, int offset);

    // push/pop genérico para expresiones binarias
    void emitPush(const SemType& t);
    void emitPop (const SemType& t, const std::string& reg); // reg: %rax o %xmm0

    // Evalúa una condición y salta a 'label' si es falsa (==0). Usado por if/while/for.
    void emitCondJumpIfFalse(Expr* cond, const std::string& label);

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
