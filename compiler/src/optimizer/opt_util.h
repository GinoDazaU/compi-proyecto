#pragma once
#include "../parser/ast.h"

// Helpers compartidos por los pases de optimización: inspección de literales.
namespace optutil {

// Valor numérico de un literal (int/float/bool) como double. true si lo es.
inline bool asDouble(Expr* e, double& out) {
    if (auto* i = dynamic_cast<IntLitExpr*>(e))   { out = static_cast<double>(i->value); return true; }
    if (auto* f = dynamic_cast<FloatLitExpr*>(e)) { out = f->value; return true; }
    if (auto* b = dynamic_cast<BoolLitExpr*>(e))  { out = b->value ? 1.0 : 0.0; return true; }
    return false;
}

// Valor entero de un literal entero (int/bool). true si lo es.
inline bool asLong(Expr* e, long long& out) {
    if (auto* i = dynamic_cast<IntLitExpr*>(e))  { out = i->value; return true; }
    if (auto* b = dynamic_cast<BoolLitExpr*>(e)) { out = b->value ? 1 : 0; return true; }
    return false;
}

inline bool isFloatLit(Expr* e) { return dynamic_cast<FloatLitExpr*>(e) != nullptr; }

inline bool isZero(Expr* e) {
    if (auto* i = dynamic_cast<IntLitExpr*>(e))   return i->value == 0;
    if (auto* f = dynamic_cast<FloatLitExpr*>(e)) return f->value == 0.0;
    if (auto* b = dynamic_cast<BoolLitExpr*>(e))  return !b->value;
    return false;
}

inline bool isOne(Expr* e) {
    if (auto* i = dynamic_cast<IntLitExpr*>(e))   return i->value == 1;
    if (auto* f = dynamic_cast<FloatLitExpr*>(e)) return f->value == 1.0;
    if (auto* b = dynamic_cast<BoolLitExpr*>(e))  return b->value;
    return false;
}

}  // namespace optutil
