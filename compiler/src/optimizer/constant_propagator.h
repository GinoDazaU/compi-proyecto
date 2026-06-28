#pragma once
#include "ast_walker.h"
#include <string>
#include <unordered_map>
#include <set>

// ─── ConstantPropagator ──────────────────────────────────────────────────────
// Propaga variables locales con valor constante conocido, sustituyendo sus usos
// por el literal. Es conservador y correcto:
//   - Solo rastrea escalares cuyo valor literal se conoce en el flujo lineal.
//   - En if/while/for guarda y restaura el estado, e invalida toda variable que
//     se asigne dentro (su valor después es incierto).
//   - Cualquier variable cuya dirección se tome (&x) nunca se propaga (podría
//     mutarse vía puntero).
class ConstantPropagator : public AstWalker {
public:
    // Valor literal conocido de una variable.
    struct LitVal {
        enum Kind { Int, Float, Bool, Char } kind;
        long long   i = 0;
        double      f = 0;
        bool        b = false;
        std::string s;  // para char
    };

    void visit(FuncDecl* node) override;
    void visit(Block* node) override;
    void visit(IfStmt* node) override;
    void visit(WhileStmt* node) override;
    void visit(ForStmt* node) override;
    void visit(VarDeclStmt* node) override;
    void visit(AssignExpr* node) override;
    void visit(UnaryExpr* node) override;
    void visit(PostfixExpr* node) override;
    void visit(IdExpr* node) override;

private:
    std::unordered_map<std::string, LitVal> consts_;  // constantes vigentes
    std::set<std::string>                   addrTaken_;  // vars con &x en la función
};
