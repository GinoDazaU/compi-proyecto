#include "sem_type.h"
#include <stdexcept>

// ─── Orden de promoción numérica ──────────────────────────────────────────────
static int numericRank(const std::string& base) {
    if (base == "bool")   return 0;
    if (base == "char")   return 1;
    if (base == "int")    return 2;
    if (base == "float")  return 3;
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
        throw std::runtime_error("promote: non-numeric types");
    return ra >= rb ? a : b;
}

SemType SemType::makeFunc(std::vector<SemType> params, const SemType& ret) {
    SemType t;
    t.base   = "fn";
    t.params = std::move(params);
    t.ret    = std::make_shared<SemType>(ret);
    return t;
}

SemType SemType::fromTypeNode(const TypeNode* node) {
    if (!node) return SemType{"void"};
    SemType t;
    t.base = node->base;
    t.mods = node->mods;
    return t;
}

std::string SemType::toString() const {
    if (base == "fn") {
        std::string s = "fn(";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i) s += ",";
            s += params[i].toString();
        }
        s += ")->";
        s += ret ? ret->toString() : "void";
        return s;
    }
    std::string s = base;
    for (auto m : mods)
        s += "*";
    return s;
}
