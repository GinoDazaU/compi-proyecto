#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>

template <typename T>
class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, T>> scopes;

public:
    SymbolTable() { enterScope(); }

    void enterScope() { scopes.emplace_back(); }

    void exitScope() {
        assert(scopes.size() > 1 && "No se puede salir del ámbito global");
        scopes.pop_back();
    }

    // Declara en el scope actual. Retorna false si ya existe en este scope.
    bool declare(const std::string& name, const T& info) {
        auto& current = scopes.back();
        if (current.count(name)) return false;
        current[name] = info;
        return true;
    }

    // Busca en todos los scopes desde el más interno. Retorna nullptr si no existe.
    T* lookup(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return &found->second;
        }
        return nullptr;
    }

    const T* lookup(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return &found->second;
        }
        return nullptr;
    }
};
