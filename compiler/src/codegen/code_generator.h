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
    std::unordered_map<std::string, CodegenStructInfo> structs_;

    // Estado durante la emisión
    int         offset_        = -8;  // offset de la próxima var local
    int         label_counter_ = 0;   // contador global de labels únicos
    int         str_counter_   = 0;   // contador de string literals
    std::string current_func_;

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

    // load/store según tipo (int→%rax, float→%xmm0, bool/char→%al+movzbq)
    void emitLoad (const SemType& t, int offset);
    void emitStore(const SemType& t, int offset);

    // push/pop genérico para expresiones binarias
    void emitPush(const SemType& t);
    void emitPop (const SemType& t, const std::string& reg); // reg: %rax o %xmm0

    // sección .data al inicio
    void emitDataSection();

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
    void visit(CastExpr* node)         override;
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
    void visit(ForRangeStmt* node)     override;
    void visit(ReturnStmt* node)       override;
    void visit(BreakStmt* node)        override;
    void visit(ContinueStmt* node)     override;
    void visit(DeleteStmt* node)       override;

    // ─── Declaraciones globales ───────────────────────────────────────────
    void visit(Program* node)          override;
    void visit(GlobalVarDecl* node)    override;
    void visit(StructDecl* node)       override;
    void visit(FuncDecl* node)         override;
    void visit(TemplateFuncDecl* node) override;
};
