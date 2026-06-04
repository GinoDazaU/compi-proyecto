#include "sem_type.h"
#include <stdexcept>

// ─── Orden de promoción numérica ──────────────────────────────────────────────
static int numericRank(const std::string& base) {
    if (base == "bool")   return 0;
    if (base == "char")   return 1;
    if (base == "int")    return 2;
    if (base == "long")   return 3;
    if (base == "float")  return 4;
    if (base == "double") return 5;
    return -1;
}

bool SemType::accepts(const SemType& other) const {
    // Mismo tipo exacto
    if (*this == other) return true;

    // Punteros: solo acepta el mismo tipo de puntero
    if (hasPointer() || other.hasPointer()) return false;

    // Promoción numérica implícita
    int myRank    = numericRank(base);
    int otherRank = numericRank(other.base);
    if (myRank >= 0 && otherRank >= 0) return otherRank <= myRank;

    return false;
}

SemType SemType::promote(const SemType& a, const SemType& b) {
    int ra = numericRank(a.base);
    int rb = numericRank(b.base);
    if (ra < 0 || rb < 0)
        throw std::runtime_error("promote: tipos no numéricos");
    return ra >= rb ? a : b;
}

SemType SemType::fromTypeNode(const TypeNode* node) {
    if (!node) return SemType{"void"};
    SemType t;
    t.base     = node->base;
    t.mods     = node->mods;
    t.is_const = node->is_const;
    return t;
}

std::string SemType::toString() const {
    std::string s;
    if (is_const) s += "const ";
    s += base;
    for (auto m : mods)
        s += (m == PtrMod::Pointer ? "*" : "&");
    return s;
}
