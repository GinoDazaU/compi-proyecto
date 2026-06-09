#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include "../parser/visitor.h"
#include "../parser/ast.h"
#include "sem_type.h"
#include "symbol_table.h"

// ─── Error semántico ──────────────────────────────────────────────────────────
struct SemanticError : std::runtime_error {
    int line, col;
    SemanticError(const std::string& msg, int line = 0, int col = 0)
        : std::runtime_error(msg), line(line), col(col) {}
};

// ─── Info de variable ─────────────────────────────────────────────────────────
struct VarInfo {
    SemType type;
    bool    is_const = false;
};

// ─── Info de parámetro ────────────────────────────────────────────────────────
struct ParamInfo {
    SemType type;
    bool    is_ref      = false;
    bool    has_default = false;
};

// ─── Info de función ─────────────────────────────────────────────────────────
struct FuncInfo {
    SemType                return_type;
    std::vector<ParamInfo> params;
    bool                   is_variadic    = false;
    std::string            template_param;  // no vacío si es función template
};

// ─── Info de struct ───────────────────────────────────────────────────────────
struct MemberInfo {
    std::string name;
    SemType     type;
};

struct StructInfo {
    std::vector<MemberInfo> members;
    const MemberInfo* find(const std::string& name) const {
        for (const auto& m : members)
            if (m.name == name) return &m;
        return nullptr;
    }
};

// ─── TypeChecker ──────────────────────────────────────────────────────────────
class TypeChecker : public Visitor {
public:
    TypeChecker();
    ~TypeChecker() override = default;

    void check(Program* program);

private:
    SymbolTable<VarInfo>                        vars_;
    std::unordered_map<std::string, FuncInfo>   funcs_;
    std::unordered_map<std::string, StructInfo> structs_;

    SemType     ret_type_;
    bool        in_loop_        = false;
    bool        in_template_    = false;
    std::string template_param_;
    SemType     expr_type_;

    void    firstPass(Program* program);
    void    registerBuiltins();

    void    semError(const std::string& msg, int line = 0, int col = 0);
    bool    isValidBase(const std::string& base) const;
    SemType resolveType(const TypeNode* node, int line = 0, int col = 0);
    bool    isLvalue(Expr* e) const;
    bool    isTemplateType(const SemType& t) const;
    bool    isArithmetic(const SemType& t) const;
    bool    isCondition(const SemType& t) const;
    SemType visitExpr(Expr* e);
    bool    bodyHasReturn(Stmt* s) const;

public:
    void visit(Program* node)          override;

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

    void visit(GlobalVarDecl* node)    override;
    void visit(StructDecl* node)       override;
    void visit(FuncDecl* node)         override;
    void visit(TemplateFuncDecl* node) override;
};
