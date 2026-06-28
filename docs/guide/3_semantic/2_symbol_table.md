# Tabla de Símbolos (SymbolTable)

Una **tabla de símbolos** responde, en cualquier punto del programa, la pregunta
"¿qué significa este nombre?". Cuando el checker ve `x` necesita saber si está
declarado y de qué tipo es; cuando el codegen ve `x` necesita saber en qué offset
del stack vive. Es la misma idea: **asociar un nombre a información**, respetando
el alcance (scope) donde ese nombre es visible.

El proyecto la implementa como una clase **genérica** en `symbol_table.h` (es toda
header, porque es un `template`). Es corta, así que la recorremos casi entera.

---

### 1. La idea: una pila de mapas

```cpp
template <typename T>
class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, T>> scopes;
public:
    SymbolTable() { enterScope(); }   // arranca con el scope global
    // ...
};
```

Cada `unordered_map` es **un scope** (un bloque, una función, el global). Se
apilan en un `vector`: el primero es el global, el último es el más interno.

```
scopes:  [ global ][ función ][ bloque if ]   ← el más interno está al final
```

Esta pila modela exactamente el alcance léxico de C/C++: un nombre declarado en un
bloque deja de existir al cerrar ese bloque.

---

### 2. Entrar y salir de scope

```cpp
void enterScope() { scopes.emplace_back(); }

void exitScope() {
    assert(scopes.size() > 1 && "No se puede salir del ámbito global");
    scopes.pop_back();
}
```

`enterScope` apila un mapa vacío; `exitScope` lo descarta (y con él, todas las
variables locales de ese bloque). El `assert` garantiza que nunca se desapile el
global: siempre queda al menos un scope. El TypeChecker llama a este par al entrar
y salir de cada `Block`, función, `for` y lambda.

---

### 3. Declarar: `declare`

```cpp
bool declare(const std::string& name, const T& info) {
    auto& current = scopes.back();        // solo el scope MÁS INTERNO
    if (current.count(name)) return false; // ya existe aquí → redeclaración
    current[name] = info;
    return true;
}
```

Lo clave es que `declare` mira **únicamente el scope actual** (`scopes.back()`).
De ahí salen dos comportamientos:

- Redeclarar el mismo nombre en el **mismo** bloque devuelve `false` → el checker
  lo convierte en error.
- Declarar un nombre que ya existe en un scope **exterior** sí se permite: es el
  **shadowing** de C++, una variable interna tapando a una externa.

---

### 4. Buscar: `lookup`

```cpp
T* lookup(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {  // de adentro hacia afuera
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;   // no existe en ningún scope visible
}
```

`lookup` recorre la pila **al revés** (`rbegin` → `rend`): del scope más interno
hacia el global, y devuelve la primera coincidencia. Por eso, si hay shadowing, la
variable local "gana" sobre la global del mismo nombre. Si no aparece en ningún
scope, devuelve `nullptr` y el checker lanza "variable no declarada".

---

### 5. Genérica: el `T` cambia por fase

La tabla no sabe **qué** guarda; lo decide quien la instancia:

- El TypeChecker usa `SymbolTable<VarInfo>`, donde `VarInfo` es básicamente un
  `SemType`. Le interesa el **tipo** de cada variable.
- El codegen usa `SymbolTable<VarEntry>`, donde `VarEntry` lleva tipo + **offset
  en el frame** (y datos de array). Le interesa **dónde** vive cada variable.

La misma mecánica de scopes sirve para ambos sin duplicar código.

> Detalle: las **variables** entran y salen de scope, por eso usan esta pila. Las
> **funciones** y **structs** son globales (se recolectan en la primera pasada),
> así que el TypeChecker los guarda en `unordered_map` aparte, no aquí. Regla
> mental: scopes anidados → `SymbolTable`; global y único → un mapa plano.
