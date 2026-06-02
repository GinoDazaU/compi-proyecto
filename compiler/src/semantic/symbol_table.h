#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

template <typename T>
class SymbolTable {
private:
    // Pila de ámbitos (scopes). El índice 0 es el ámbito global.
    std::vector<std::unordered_map<std::string, T>> scopes;

public:
    SymbolTable() {
        enterScope();
    }

    void clear() {
        scopes.clear();
        enterScope();
    }

    // ─── Scope Management ──────────────────────────────────────────────────────────

    void enterScope() {
        scopes.emplace_back();
    }

    void exitScope() {
        if (scopes.size() <= 1) {
            throw std::runtime_error("Error: No se puede salir del ambito global.");
        }
        scopes.pop_back();
    }

    // ─── Symbol Management ─────────────────────────────────────────────────────────

    bool declare(const std::string& name, const T& info) {
        if (scopes.empty()) return false;
        auto& currentScope = scopes.back();
        if (currentScope.find(name) != currentScope.end()) {
            return false;
        }
        currentScope[name] = info;
        return true;
    }

    T* lookup(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto search = it->find(name);
            if (search != it->end()) {
                return &(search->second);
            }
        }
        return nullptr;
    }

    const T* lookup(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto search = it->find(name);
            if (search != it->end()) {
                return &(search->second);
            }
        }
        return nullptr;
    }

    bool containsInCurrentScope(const std::string& name) const {
        if (scopes.empty()) return false;
        return scopes.back().find(name) != scopes.back().end();
    }

    bool contains(const std::string& name) const {
        return lookup(name) != nullptr;
    }

    bool update(const std::string& name, const T& info) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto search = it->find(name);
            if (search != it->end()) {
                search->second = info;
                return true;
            }
        }
        return false;
    }
};
