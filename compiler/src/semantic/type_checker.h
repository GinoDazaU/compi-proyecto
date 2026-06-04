#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include "../parser/visitor.h"
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
    SemType     type;
    bool        is_ref   = false;
    bool        has_default = false;
};

// ─── Info de función ─────────────────────────────────────────────────────────
struct FuncInfo {
    SemType              return_type;
    std::vector<ParamInfo> params;
    bool                 is_variadic = false;  // para print/println
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
    ~TypeChecker() override;
};
