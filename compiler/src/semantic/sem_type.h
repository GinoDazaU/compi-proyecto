#pragma once
#include <string>
#include <vector>
#include "../parser/ast.h"  // PtrMod

struct SemType {
    std::string         base;       // "int", "float", "void", nombre de struct, etc.
    std::vector<PtrMod> mods;       // modificadores en orden: *, &

    SemType() = default;
    explicit SemType(std::string b) : base(std::move(b)) {}
    SemType(std::string b, std::vector<PtrMod> m) : base(std::move(b)), mods(std::move(m)) {}

    // ─── Consultas ────────────────────────────────────────────────────────────
    bool isVoid()      const { return base == "void" && mods.empty(); }
    bool isNumeric()   const { return !hasPointer() && (base=="int"||base=="float"); }
    bool isIntegral()  const { return !hasPointer() && base=="int"; }
    bool isBool()      const { return base == "bool" && mods.empty(); }
    bool hasPointer()  const { return !mods.empty() && mods.back() == PtrMod::Pointer; }
    bool hasRef()      const { return !mods.empty() && mods.back() == PtrMod::Reference; }

    // Tipo al que apunta/referencia (quita el último mod)
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

    bool operator==(const SemType& o) const { return base == o.base && mods == o.mods; }
    bool operator!=(const SemType& o) const { return !(*this == o); }
};
