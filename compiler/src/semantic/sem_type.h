#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../parser/ast.h"  // PtrMod

struct SemType {
    std::string         base;       // "int", "float", "void", struct, o "fn" (función)
    std::vector<PtrMod> mods;       // modificadores en orden: *, &

    // Solo cuando base == "fn" (lambda/función como valor): la firma.
    // Una captura NO cambia esto — el tipo es el mismo capture o no.
    std::vector<SemType>     params;   // tipos de los parámetros
    std::shared_ptr<SemType> ret;      // tipo de retorno

    SemType() = default;
    explicit SemType(std::string b) : base(std::move(b)) {}
    SemType(std::string b, std::vector<PtrMod> m) : base(std::move(b)), mods(std::move(m)) {}

    // Construye un tipo función (base == "fn") a partir de params + retorno.
    static SemType makeFunc(std::vector<SemType> params, const SemType& ret);

    // ─── Consultas ────────────────────────────────────────────────────────────
    bool isVoid()      const { return base == "void" && mods.empty(); }
    bool isFunc()      const { return base == "fn"; }
    bool isNumeric()   const { return !hasPointer() && (base=="int"||base=="float"); }
    bool isIntegral()  const { return !hasPointer() && base=="int"; }
    bool isFloat()     const { return !hasPointer() && base=="float"; }
    bool isBool()      const { return base == "bool" && mods.empty(); }
    bool hasPointer()  const { return !mods.empty() && mods.back() == PtrMod::Pointer; }

    // Tipo al que apunta (quita el último mod)
    SemType deref() const {
        SemType t = *this;
        if (!t.mods.empty()) t.mods.pop_back();
        return t;
    }

    // Compatibilidad: ¿se puede asignar/pasar 'other' donde se espera 'this'?
    bool accepts(const SemType& other) const;

    // Promoción numérica entre dos tipos numéricos (devuelve el "mayor")
    static SemType promote(const SemType& a, const SemType& b);

    // Convierte un TypeNode del AST a SemType
    static SemType fromTypeNode(const TypeNode* node);

    std::string toString() const;

    bool operator==(const SemType& o) const {
        if (base != o.base || mods != o.mods) return false;
        if (base == "fn") {
            if (params.size() != o.params.size()) return false;
            for (size_t i = 0; i < params.size(); ++i)
                if (!(params[i] == o.params[i])) return false;
            if (static_cast<bool>(ret) != static_cast<bool>(o.ret)) return false;
            if (ret && !(*ret == *o.ret)) return false;
        }
        return true;
    }
    bool operator!=(const SemType& o) const { return !(*this == o); }
};
