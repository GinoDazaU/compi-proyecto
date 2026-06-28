# Constant Propagation (ConstantPropagator)

El *constant folding* (`2_constant_folding.md`) solo pliega operaciones entre
literales que ya están juntos en el árbol. Pero gran parte del código real
esconde sus constantes detrás de variables:

```cpp
int x = 5;
int y = x + 1;   // el folder solo no puede tocar esto: 'x' es una variable
```

La **propagación de constantes** cierra esa brecha: si sabe que `x` vale `5` en
ese punto, **sustituye el uso de `x` por el literal `5`**, dejando `5 + 1` — que el
siguiente paso de folding ya puede plegar a `6`. Por eso el pass manager corre
`ConstantFolder → ConstantPropagator → ConstantFolder`: la propagación abre el
trabajo que el segundo folding remata.

Lo implementa `ConstantPropagator` (en `constant_propagator.h` / `.cpp`). Es el
pase más delicado del optimizador, porque rastrear "qué vale una variable aquí"
exige seguir el flujo del programa con cuidado para no equivocarse nunca.

---

## 1. El estado: qué variables son constantes

El pase mantiene dos estructuras durante el recorrido:

```cpp
std::unordered_map<std::string, LitVal> consts_;     // constantes vigentes ahora
std::set<std::string>                   addrTaken_;  // vars con &x en la función
```

`consts_` mapea el nombre de cada variable que **en este punto del flujo** tiene un
valor literal conocido, a ese valor. `LitVal` es un literal etiquetado con su tipo:

```cpp
struct LitVal {
    enum Kind { Int, Float, Bool, Char } kind;
    long long i; double f; bool b; std::string s;  // según kind
};
```

`consts_` no es un mapa estático de "constantes del programa": es un estado que
**cambia mientras se recorre**. Una variable entra cuando se le asigna un literal y
sale en cuanto deja de ser segura asumir su valor.

---

## 2. Sustituir el uso: `visit(IdExpr)`

El reemplazo en sí es la parte fácil. Cuando el recorrido llega a un uso de
variable, se consulta `consts_`:

```cpp
void ConstantPropagator::visit(IdExpr* node) {
    auto it = consts_.find(node->name);
    if (it != consts_.end()) replaceWith(makeLiteral(it->second));
}
```

Si la variable es constante vigente, `replaceWith` la cambia por un literal fresco
(`makeLiteral` construye el `IntLitExpr` / `FloatLitExpr` / … según el `kind`). Toda
la dificultad del pase no está aquí, sino en mantener `consts_` **siempre
correcto**: que una variable esté en el mapa solo si de verdad vale eso.

---

## 3. Registrar una constante: `visit(VarDeclStmt)`

Una variable entra a `consts_` cuando se declara con un inicializador literal, y
solo si se cumplen varias condiciones de seguridad:

```cpp
LitVal v;
if (node->dimensions.empty() && node->init && !addrTaken_.count(node->name) &&
    fromLiteral(node->init, v) && typeMatches(node->type, v)) {
    consts_[node->name] = v;
} else {
    consts_.erase(node->name);  // sombrea cualquier constante externa homónima
}
```

Las condiciones, una por una:

- **`dimensions.empty()`** — solo escalares; no se propagan arrays.
- **`fromLiteral(node->init, v)`** — el inicializador debe ser un literal directo.
- **`!addrTaken_.count(...)`** — la variable no aparece en ningún `&x` (sección 5).
- **`typeMatches(node->type, v)`** — el tipo del literal coincide con el declarado.

El `else` es tan importante como el `if`: si una variable nueva **no** califica como
constante, igual hay que `erase`-arla, por si en un scope exterior había otra del
mismo nombre que sí era constante. La declaración interna la **sombrea**, y su valor
no debe propagarse aquí.

### Por qué `typeMatches` importa

```cpp
float x = 5;   // literal int '5', pero x es float
```

El literal es `int`, pero la variable es `float`. El codegen, al ver `float x = 5`,
aplica la promoción `int → float`. Si propagáramos el `5` crudo a un uso posterior
de `x`, perderíamos esa conversión y cambiaríamos el tipo de la expresión. Por eso
`typeMatches` exige que el tipo del literal **calce** con el declarado (o que la
variable sea `auto`, donde el tipo es justamente el del literal):

```cpp
static bool typeMatches(TypeNode* t, const LitVal& v) {
    if (!t || !t->mods.empty()) return false;  // sin tipo o puntero: nunca
    if (t->is_auto) return true;               // auto: el tipo es el del literal
    switch (v.kind) {
        case LitVal::Int:   return t->base == "int";
        case LitVal::Float: return t->base == "float";
        // ... bool, char ...
    }
}
```

---

## 4. Invalidar: cuando una variable deja de ser constante

Una variable sale de `consts_` apenas su valor puede cambiar. Las reasignaciones
y mutaciones la invalidan:

```cpp
void ConstantPropagator::visit(AssignExpr* node) {
    walk(node->right);  // propaga primero en el lado derecho (rvalue)
    if (auto* id = dynamic_cast<IdExpr*>(node->left)) {
        consts_.erase(id->name);   // x = ... : x deja de ser la constante anterior
    } else {
        walk(node->left);          // lvalue compuesto (arr[k], s.m): propaga sub-rvalues
    }
}
```

Nótese el orden: primero se propaga en el **lado derecho** (donde la variable aún
vale lo viejo), y recién después se invalida el destino. Y un detalle de diseño
explícito: aunque `x = 7` le da a `x` un valor literal nuevo, el pase **no** lo
registra como constante — solo lo borra. La razón es que en una `AssignExpr` no se
conoce el tipo declarado de `x`, que `typeMatches` necesitaría para no perder una
conversión. Conservador: ante la duda, no propagar.

Lo mismo ocurre con los operadores que mutan una variable in situ, `++` / `--`:

```cpp
void ConstantPropagator::visit(UnaryExpr* node) {
    if (node->op == AddrOf || node->op == PreInc || node->op == PreDec) {
        if (auto* id = dynamic_cast<IdExpr*>(node->expr)) consts_.erase(id->name);
        else walk(node->expr);
    } else {
        walk(node->expr);  // Neg / Not / Deref: el operando es rvalue normal
    }
}
void ConstantPropagator::visit(PostfixExpr* node) {
    if (auto* id = dynamic_cast<IdExpr*>(node->base)) consts_.erase(id->name);
    else walk(node->base);
}
```

`++x`, `x--`, etc. invalidan `x`. Y se evita propagar dentro del operando de estos
operadores: no tendría sentido sustituir `x` por un literal y luego intentar
`++5`.

---

## 5. La trampa de los punteros: `addrTaken_`

Hay una forma silenciosa de cambiar una variable: tomar su dirección y mutarla por
el puntero.

```cpp
int x = 5;
int* p = &x;
*p = 99;          // x cambió, ¡pero no hay ningún 'x = ...' a la vista!
print(x);         // vale 99, NO 5
```

Si propagáramos `x` como `5`, romperíamos el programa. Como detectar esto durante
el flujo lineal es frágil, el pase usa una regla tajante: **cualquier variable cuya
dirección se tome alguna vez en la función jamás se propaga**. Antes de recorrer el
cuerpo, un pre-pase recolecta todos los `&x`:

```cpp
void ConstantPropagator::visit(FuncDecl* node) {
    consts_.clear();
    addrTaken_.clear();
    AddrTakenCollector c(addrTaken_);
    node->body->accept(&c);     // 1ª pasada: junta todas las vars con &
    node->body->accept(this);   // 2ª pasada: propaga, saltando las de addrTaken_
}
```

`AddrTakenCollector` es un `AstWalker` de solo lectura que recorre el cuerpo y
anota cada `&x`. Con `addrTaken_` lleno, `visit(VarDeclStmt)` ya nunca registra una
variable peligrosa (sección 3).

---

## 6. El flujo del control: lo más sutil

Las constantes fluyen limpio en código **lineal**, pero el control de flujo rompe
esa linealidad: tras un `if`, no se sabe qué rama se ejecutó; en un `while`, el
cuerpo puede correr varias veces. El pase trata cada construcción con cuidado.

### Bloques

```cpp
void ConstantPropagator::visit(Block* node) {
    std::vector<std::string> declared;
    for (Stmt* s : node->stmts) {
        s->accept(this);
        if (auto* vd = dynamic_cast<VarDeclStmt*>(s)) declared.push_back(vd->name);
    }
    for (const auto& n : declared) consts_.erase(n);  // salen de alcance
}
```

Dentro del bloque las constantes fluyen de una sentencia a la siguiente; al
cerrarlo, las variables declaradas **dentro** se descartan, porque fuera ya no
existen.

### `if`: cada rama parte del mismo estado

```cpp
void ConstantPropagator::visit(IfStmt* node) {
    walk(node->condition);            // la condición corre con las constantes actuales

    auto saved = consts_;             // guardar el estado de antes del if
    node->then_branch->accept(this);
    consts_ = saved;                  // restaurar para la rama else
    if (node->else_branch) {
        node->else_branch->accept(this);
        consts_ = saved;
    }

    // Tras el if, toda variable asignada en CUALQUIER rama queda incierta:
    std::set<std::string> mod;
    collectAssigned(node->then_branch, mod);
    if (node->else_branch) collectAssigned(node->else_branch, mod);
    for (const auto& v : mod) consts_.erase(v);
}
```

Dos ideas: cada rama se analiza partiendo del estado **anterior** al `if`
(guardado en `saved`), porque las dos son alternativas, no secuencia. Y al salir,
como no se sabe qué rama corrió, **cualquier variable modificada en alguna rama**
deja de ser constante. `collectAssigned` (un `AstWalker` de solo lectura) junta
esos nombres.

### `while` y `for`: invalidar *antes* de entrar

Un bucle es traicionero porque su cuerpo puede repetir: una variable que el cuerpo
modifica ya no es la de antes **ni siquiera en la condición** de la segunda vuelta.
Por eso se invalida lo modificado **antes** de analizar nada:

```cpp
void ConstantPropagator::visit(WhileStmt* node) {
    std::set<std::string> mod;
    collectAssigned(node->condition, mod);
    collectAssigned(node->body, mod);
    for (const auto& v : mod) consts_.erase(v);  // invalidar ANTES

    walk(node->condition);
    auto saved = consts_;
    node->body->accept(this);
    consts_ = saved;
}
```

`for` es igual, con su `init` analizado primero (y la variable del init descartada
al final, porque su alcance es el bucle).

---

## 7. Los colectores auxiliares

Dos veces apareció "un `AstWalker` de solo lectura". Son clases internas
(`AssignedCollector`, `AddrTakenCollector`) que **no reescriben** el árbol: lo
recorren juntando nombres en un `set`. Reusan el recorrido de `AstWalker` y solo
sobreescriben los nodos relevantes:

```cpp
class AssignedCollector : public AstWalker {
    void visit(AssignExpr* n) override {
        if (auto* id = dynamic_cast<IdExpr*>(n->left)) out_.insert(id->name);
        AstWalker::visit(n);
    }
    void visit(UnaryExpr* n) override { /* AddrOf, PreInc, PreDec → out_ */ ... }
    void visit(PostfixExpr* n) override { /* x++ / x-- → out_ */ ... }
    void visit(VarDeclStmt* n) override { out_.insert(n->name); AstWalker::visit(n); }
};
```

Es el mismo `AstWalker` de `1_optimizer.md`, usado en modo "inspección" en vez de
"reescritura": un buen ejemplo de cómo la clase base sirve para recorridos que solo
leen, no solo para los que transforman.

---

## En contexto

El `ConstantPropagator` es la pieza que conecta las declaraciones con sus usos, y
su valor real solo aparece **entre** dos pasadas de `ConstantFolder`
(`2_constant_folding.md`): propaga literales que el folding luego pliega. Su
disciplina —salvar/restaurar estado en cada control de flujo, invalidar ante
asignaciones, punteros y bucles— es lo que garantiza que nunca cambie el
comportamiento del programa, la regla de oro del optimizador (`1_optimizer.md §6`).
