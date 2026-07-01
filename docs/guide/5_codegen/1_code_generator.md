# Generación de Código (CodeGenerator)

El **Generador de Código** (CodeGenerator) es la última fase del compilador. Recibe
el AST ya validado por el semántico y lo traduce a **código ensamblador x86-64**
(sintaxis AT&T), que luego `g++` ensambla y enlaza para producir el ejecutable. No
hay representación intermedia: se va del AST directo al assembly, recorriéndolo con
el mismo patrón Visitor que usó el TypeChecker.

Vamos a ver cómo está diseñado y cómo funciona cada parte en `code_generator.h` y
`code_generator.cpp`.

Antes de entrar, un detalle de diseño que aparece en **todo** el archivo: como
`visit(...)` devuelve `void`, un `visit` de expresión no puede *retornar* lo que
calculó. Por convención, cada expresión deja su **valor** en un registro fijo
(`%rax` para enteros, `bool`, `char` y punteros; `%xmm0` para `float`) y su **tipo**
en el miembro `cur_type_`, que el nodo padre consulta. Es exactamente el mismo
mecanismo del `expr_type_` del semántico: cuando leas `node->left->accept(this)`,
piensa "después de esto el valor está en `%rax`/`%xmm0` y su tipo en `cur_type_`".

---

### 1. La estructura de code_generator.h

La clase hereda de `Visitor` y mantiene todo su estado en miembros privados:

```cpp
class CodeGenerator : public Visitor {
    std::ostream& out_;                 // a dónde se escribe el .s

    SymbolTable<VarEntry> env_;         // variables visibles → dónde viven

    // Precalculados en la primera pasada:
    std::unordered_map<std::string, int>                  frame_sizes_;  // func → bytes de frame
    std::unordered_map<std::string, SemType>              func_rets_;    // func → tipo de retorno
    std::unordered_map<std::string, std::vector<SemType>> func_params_;  // func → tipos de params
    std::unordered_map<std::string, CodegenStructInfo>    structs_;      // struct → layout

    int offset_        = -8;            // offset de la próxima variable local
    int label_counter_ = 0;            // para generar labels únicos
    int str_counter_, float_counter_;  // para nombrar literales en .rodata
    std::string current_func_;         // función que se está generando

    SemType cur_type_;                 // tipo de la última expresión (la convención de arriba)
    bool    cur_array_decay_ = false;  // ¿lo que hay en %rax es la dirección de un sub-array?

    std::stack<std::pair<std::string,std::string>> loop_labels_;  // para break/continue
    std::vector<...> string_literals_, float_literals_;           // constantes diferidas
};
```

Lo importante de cada grupo:

- **`env_`** es la misma `SymbolTable` genérica del semántico, pero instanciada con
  `VarEntry`. Al semántico le importaba el *tipo* de cada variable; al codegen le
  importa **dónde** vive (su offset en el frame).
- **`frame_sizes_`, `func_rets_`, `func_params_`, `structs_`** se llenan en la
  primera pasada para tener esa información lista antes de emitir.
- **`offset_`** es el contador que va asignando posiciones a las variables locales.
- **`cur_type_` / `cur_array_decay_`** son el "canal de retorno" de cada `visit` de
  expresión.
- **`loop_labels_`** es una pila con las etiquetas del loop actual, para que
  `break`/`continue` sepan a dónde saltar.

Dos structs auxiliares acompañan a la clase:

```cpp
struct VarEntry {            // lo que env_ guarda por cada variable
    SemType type;
    int     offset   = 0;    // offset desde %rbp
    bool    is_array = false; // array/struct: su "valor" es su dirección base, no un load
    std::vector<int> dims;   // tamaños por dimensión (arrays multidimensionales)
};

struct CodegenStructInfo {   // el layout de un struct
    std::unordered_map<std::string,int>     offsets; // miembro → offset desde la base
    std::unordered_map<std::string,SemType> types;   // miembro → tipo
    int size = 0;
};
```

---

### 2. El punto de entrada: `gencode`

```cpp
void CodeGenerator::gencode(Program* program) {
    firstPass(program);

    // Generar el .text a un buffer temporal
    std::ostringstream body;
    std::streambuf* prev = out_.rdbuf(body.rdbuf());
    program->accept(this);      // recorre el AST y emite el código de las funciones
    out_.rdbuf(prev);

    emitDataSection();          // .data (format strings de printf)
    emitRodataSection();        // .rodata (literales float y string)
    out_ << ".text\n";
    out_ << body.str();         // ahora sí, el código generado

    out_ << ".section .note.GNU-stack,\"\",@progbits\n";
}
```

El orden parece raro: primero se genera el código a un **buffer**
(`std::ostringstream`), y recién después se escriben las secciones de datos. La
razón es que los literales `float` y `string` solo se descubren **mientras** se
recorre el cuerpo (cuando aparece un `3.14` o un `"hola"`), pero en el archivo deben
ir declarados *antes* en `.rodata`. Así que se redirige temporalmente el buffer del
`ostream` con `rdbuf`, se genera todo el `.text` a memoria, y al terminar —cuando ya
se conocen todas las constantes— se escriben primero los datos y luego el código.

---

### 3. La primera pasada: `firstPass`

El CodeGenerator es **autosuficiente**: no usa nada que dejara el TypeChecker (el
AST no viene anotado), recalcula por su cuenta lo que necesita.

```cpp
void CodeGenerator::firstPass(Program* program) {
    // 1º: layouts de struct (los frames los necesitan para dimensionar variables struct)
    for (auto d : program->decls)
        if (auto* s = dynamic_cast<StructDecl*>(d)) buildStructInfo(s);

    // 2º: por cada función, su tamaño de frame, su retorno y los tipos de sus params
    for (auto d : program->decls)
        if (auto* f = dynamic_cast<FuncDecl*>(d)) {
            frame_sizes_[f->name] = frameSize(f);
            func_rets_[f->name]   = SemType::fromTypeNode(f->return_type);
            std::vector<SemType> ptypes;
            for (auto& p : f->params) ptypes.push_back(SemType::fromTypeNode(p.type));
            func_params_[f->name] = std::move(ptypes);
        }
}
```

`func_rets_` se guarda para, tras una llamada, dejar `cur_type_` con el tipo de
retorno correcto; `func_params_` para promover cada argumento al tipo del parámetro
antes de pasarlo.

#### A. `buildStructInfo` — el layout de un struct

```cpp
void CodeGenerator::buildStructInfo(StructDecl* s) {
    CodegenStructInfo info;
    int off = 0;
    for (auto& m : s->members) {
        info.offsets[m.name] = off;                    // offset desde la base del struct
        info.types[m.name]   = SemType::fromTypeNode(m.type);
        off += 8;                                       // cada miembro, un slot de 8 bytes
    }
    info.size = off;
    structs_[s->name] = info;
}
```

Cada miembro ocupa un slot de 8 bytes y se ubica en un offset **positivo** desde la
base del struct (`0, 8, 16…`). Positivo porque, teniendo la dirección base en un
registro, los miembros van hacia adelante; y así la misma fórmula `base + offset`
sirve para un struct en el stack o en el heap.

#### B. `frameSize`, `declSlots`, `arrayElemCount` — cuánto reservar

El prólogo de una función reserva de una vez todo el espacio de sus variables. Para
saber cuánto, `frameSize` cuenta los slots recorriendo el cuerpo:

```cpp
int CodeGenerator::frameSize(FuncDecl* f) {
    int slots = (int)f->params.size();              // los parámetros también se guardan

    std::function<void(Stmt*)> countStmt = [&](Stmt* s) {
        if (auto* vd = dynamic_cast<VarDeclStmt*>(s))   slots += declSlots(vd);
        else if (auto* b = dynamic_cast<Block*>(s))     for (auto i : b->stmts) countStmt(i);
        else if (auto* i = dynamic_cast<IfStmt*>(s))  { countStmt(i->then_branch); countStmt(i->else_branch); }
        else if (auto* w = dynamic_cast<WhileStmt*>(s)) countStmt(w->body);
        else if (auto* fr = dynamic_cast<ForStmt*>(s)) { if (fr->init.decl) slots += declSlots(fr->init.decl);
                                                          countStmt(fr->body); }
    };
    countStmt(f->body);

    int bytes = slots * 8;
    if (bytes % 16 != 0) bytes += 16 - (bytes % 16);   // alinear a 16 (lo exige el ABI)
    return bytes;
}
```

Es un mini-recorrido aparte que entra a bloques, `if` y loops sumando los slots de
cada declaración. `declSlots` decide cuántos reserva cada `VarDeclStmt`: un array,
tantos como elementos (`arrayElemCount` multiplica las dimensiones); un struct, sus
`size/8` slots; un escalar, 1.

#### C. Caso del nombre repetido y otros

`frameSize` y `buildStructInfo` no validan nada: para cuando el codegen corre, el
semántico ya garantizó que el programa es correcto. Por eso aquí no hay manejo de
errores; el codegen confía y solo traduce.

---

### 4. Los helpers de emisión

Antes de los `visit`, conviene conocer las piezas reutilizables que casi todos usan.

#### A. Cargar y guardar según el tipo

`emitLoad`/`emitStore` mueven entre un slot del frame (`offset(%rbp)`) y el registro
del tipo. Toda la lógica "qué instrucción según el tipo" vive aquí:

```cpp
void CodeGenerator::emitLoad(const SemType& t, int offset) {
    if (t.isFloat())          out_ << "    movsd " << offset << "(%rbp), %xmm0\n";
    else if (t.isByteSized()) out_ << "    movb " << offset << "(%rbp), %al\n"
                                   << "    movzbq %al, %rax\n";   // bool/char a 64 bits limpios
    else                      out_ << "    movq " << offset << "(%rbp), %rax\n";
}
```

Sus primos `emitLoadIndirect`/`emitStoreIndirect` hacen lo mismo pero cuando la
dirección ya está calculada en un registro (`(%rax)`), no en un offset fijo — para
`arr[i]`, `*p`, `s.x`.

#### B. Apilar valores: `emitPush` / `emitPop`

Las binarias necesitan guardar el operando izquierdo mientras evalúan el derecho.
Para enteros es un `pushq`; los `float`, que viven en `%xmm0`, requieren bajar
`%rsp` a mano:

```cpp
void CodeGenerator::emitPush(const SemType& t) {
    if (t.isFloat()) out_ << "    subq $8, %rsp\n    movsd %xmm0, (%rsp)\n";
    else             out_ << "    pushq %rax\n";
}
```

#### C. Promoción `int → float`: `emitPromote`

El semántico permite mezclar `int` y `float` pero no toca el AST, así que la
conversión real la hace el codegen, donde haga falta (asignación, argumento,
`return`):

```cpp
void CodeGenerator::emitPromote(const SemType& target) {
    if (target.isFloat() && !cur_type_.isFloat() && !cur_type_.hasPointer()) {
        out_ << "    cvtsi2sdq %rax, %xmm0\n";
        cur_type_ = SemType{"float"};
    }
}
```

#### D. Convertir un valor en una decisión

Tres helpers transforman "un valor" en "un salto":

- `emitCompareZero(t)` — compara el valor contra 0 (con `cmpq` si es entero, o
  `ucomisd` contra 0 si es float) y deja las *flags* listas.
- `emitToBool(t)` — normaliza el valor a 0/1 en `%rax`.
- `emitSetccBool(op, floatCmp)` — tras una comparación ya emitida, deja el 0/1 del
  `set<cc>` correspondiente; el mapeo operador → instrucción está en el helper
  estático `setccFor`.
- `emitCondJumpIfFalse(cond, label)` — el que usan `if`/`while`/`for`: evalúa la
  condición y salta a `label` si resultó falsa.

#### E. Labels y constantes

`nextLabel()` da un entero único y `label("else", n)` arma `"__else_<n>"` (el
prefijo `__` evita chocar con nombres del usuario). `floatLabel(v)` y `strLabel(s)`
registran una constante en sus vectores —con deduplicación, reusando el label si el
valor ya existía— y devuelven su etiqueta `__float_N` / `__str_N`, que más tarde
`emitRodataSection` vuelca a `.rodata`.

---

### 5. Funciones: `visit(FuncDecl)`

Aquí está el esqueleto de toda función generada — prólogo, parámetros, cuerpo,
epílogo:

```cpp
void CodeGenerator::visit(FuncDecl* node) {
    current_func_ = node->name;
    offset_       = -8;            // la primera variable local irá en -8(%rbp)
    env_.enterScope();

    out_ << ".globl " << node->name << "\n" << node->name << ":\n";
    out_ << "    pushq %rbp\n    movq %rsp, %rbp\n";       // prólogo

    int frame = frame_sizes_[node->name];
    if (frame > 0) out_ << "    subq $" << frame << ", %rsp\n";  // reservar locales

    // Copiar los parámetros (que llegan en registros) a sus slots del frame
    int nInt = 0, nFloat = 0;
    for (auto& p : node->params) {
        SemType pt = SemType::fromTypeNode(p.type);
        int off = offset_;
        env_.declare(p.name, VarEntry{pt, off});
        offset_ -= 8;
        if (pt.isFloat()) { if (nFloat < 8) out_ << "    movsd " << FLOAT_ARG_REGS[nFloat] << ", " << off << "(%rbp)\n"; ++nFloat; }
        else              { if (nInt   < 6) out_ << "    movq "  << INT_ARG_REGS[nInt]    << ", " << off << "(%rbp)\n"; ++nInt; }
    }

    node->body->accept(this);

    out_ << ".end_" << node->name << ":\n    leave\n    ret\n";   // epílogo
    env_.exitScope();
}
```

Lo que hay que notar del código:

- `offset_` se reinicia a `-8` por función y baja de 8 en 8; los offsets son
  negativos porque las locales viven debajo de `%rbp`.
- Los **parámetros** llegan en los registros de la convención (`INT_ARG_REGS`,
  `FLOAT_ARG_REGS`, dos bancos con índice propio) y lo primero que hace la función
  es copiarlos a slots del frame, para tratarlos igual que cualquier variable. Se
  registran en `env_`.
- El epílogo es **una sola etiqueta** `.end_<func>`: cualquier `return` salta ahí
  (sección 8), y ahí `leave`/`ret` cierran el frame.

---

### 6. Declaración de variables: `visit(VarDeclStmt)`

Reserva espacio en el frame (avanzando `offset_`) y, si hay inicializador, emite el
store. Hay tres casos.

**Escalar** (lo habitual):

```cpp
int off = offset_;
offset_ -= 8;
if (node->init) {
    node->init->accept(this);                       // valor → %rax/%xmm0
    SemType t = node->type->is_auto ? cur_type_     // auto: toma el tipo del inicializador
                                    : SemType::fromTypeNode(node->type);
    emitPromote(t);                                 // int→float si el destino es float
    env_.declare(node->name, VarEntry{t, off});
    emitStore(t, off);
}
```

Nota cómo `auto` se resuelve solo: el codegen toma el `cur_type_` que dejó el
inicializador, sin inferir nada más (el semántico ya validó que se podía).

**Array estático** (`int arr[5]`, `int m[2][3]`): reserva todos los slots de una vez,
registra la variable como `is_array` decaída a puntero, y guarda las dimensiones
para indexar después:

```cpp
int count = arrayElemCount(node);
int base  = offset_ - (count - 1) * 8;    // el slot más bajo = arr[0]
offset_  -= count * 8;

SemType ptr = SemType::fromTypeNode(node->type);
std::vector<int> dims;
for (auto* d : node->dimensions) {
    ptr.mods.push_back(PtrMod::Pointer);              // decae a T* (T** si 2D)
    auto* lit = dynamic_cast<IntLitExpr*>(d);
    dims.push_back(lit ? (int)lit->value : 0);
}
env_.declare(node->name, VarEntry{ptr, base, /*is_array=*/true, dims});
// (si hay init_list {1,2,3}, se hace un emitStore por elemento)
```

**Variable struct** (`Punto p`): reserva `size` bytes y, como el array, la variable
"vale" su dirección base (`is_array = true`). No tiene inicializador porque la
gramática no tiene literales de struct.

---

### 7. Direcciones de lvalue: `emitLvalueAddr`

Esta es la función más densa. Un *lvalue* es algo asignable o al que se le puede
tomar dirección: `x`, `arr[i]`, `*p`, `s.x`. `emitLvalueAddr` deja en `%rax` **la
dirección** de ese lvalue (no su valor) y en `cur_type_` el tipo de lo que vive ahí.
La usan la asignación, `&`, `++`/`--` y la lectura de índices y miembros.

Maneja cuatro formas. La de un identificador es directa (`leaq` de su slot). La de
**indexado** es la interesante, porque separa arrays estáticos de punteros/strings.
Primero aplana la cadena de índices para hallar la raíz:

```cpp
std::vector<Expr*> idxs;
Expr* root = ix;
while (auto* inner = dynamic_cast<IndexExpr*>(root)) {   // m[i][j] → root=m, idxs=[j,i]
    idxs.push_back(inner->index);
    root = inner->base;
}
std::reverse(idxs.begin(), idxs.end());                  // idxs = [i, j]
```

Si la raíz es un **array estático** (tiene `dims`), calcula un índice lineal
row-major por el método de Horner y lo suma a la base:

```cpp
if (en && en->is_array && idxs.size() <= en->dims.size()) {
    idxs[0]->accept(this);                          // i0 → %rax
    for (size_t p = 1; p < k; ++p) {                // acc = acc*dim[p] + ip
        out_ << "    imulq $" << dims[p] << ", %rax\n    pushq %rax\n";
        idxs[p]->accept(this);
        out_ << "    popq %rcx\n    addq %rcx, %rax\n";
    }
    int tail = 1; for (size_t p = k; p < n; ++p) tail *= dims[p];   // índice parcial
    out_ << "    imulq $" << tail*8 << ", %rax\n";       // índice → bytes
    out_ << "    leaq " << baseOff << "(%rbp), %rcx\n    addq %rcx, %rax\n";
    for (size_t p = 0; p < k; ++p) baseTy = baseTy.deref();
    cur_type_        = baseTy;
    cur_array_decay_ = (k < n);   // quedan dimensiones → es un sub-array (dirección = valor)
    return;
}
```

Para `int m[2][3]`, `m[i][j]` da `base + (i*3 + j)*8`. El `tail` cubre el indexado
**parcial** (`m[i]`, una fila): el resultado es la dirección de un sub-array, y se
marca `cur_array_decay_` para que nadie le haga un `load`.

Si la raíz **no** es array estático (es un puntero de `new`, o un `string`), se hace
un solo nivel con el *stride* adecuado — 8 para punteros, **1 para strings** (sus
bytes están empaquetados):

```cpp
ix->base->accept(this);                  // puntero base → %rax
bool isStr = (cur_type_.base == "string");
out_ << "    pushq %rax\n";
ix->index->accept(this);
out_ << "    movq %rax, %rcx\n    popq %rax\n";
if (!isStr) out_ << "    imulq $8, %rcx\n";
out_ << "    addq %rcx, %rax\n";
cur_type_ = isStr ? SemType{"char"} : cur_type_.deref();
```

Las otras dos formas: `*p` como lvalue es trivial (el valor del puntero ya es la
dirección destino), y `s.x` / `p->x` toman la dirección base del struct y le suman
el offset del miembro.

---

### 8. Sentencias

#### A. `visit(Block)`, `visit(ExprStmt)`

`Block` abre un scope en `env_`, recorre sus sentencias y lo cierra. `ExprStmt`
simplemente evalúa la expresión (descartando su valor).

#### B. `visit(ReturnStmt)`

```cpp
void CodeGenerator::visit(ReturnStmt* node) {
    if (node->expr) {
        node->expr->accept(this);                    // resultado en %rax/%xmm0
        emitPromote(func_rets_[current_func_]);      // int→float si la función retorna float
    }
    out_ << "    jmp .end_" << current_func_ << "\n";
}
```

No emite `ret` directamente: salta a la etiqueta `.end_<func>`, el único punto de
salida donde está el epílogo. Así un `return` en medio de la función igual cierra el
frame correctamente.

#### C. Control de flujo: `if`, `while`, `for`

Todos comparten el patrón "evaluar condición, comparar con 0, saltar", con labels
únicos. `if`:

```cpp
if (node->else_branch) {
    emitCondJumpIfFalse(node->condition, elseLabel);   // falso → else
    node->then_branch->accept(this);
    out_ << "    jmp " << endLabel << "\n" << elseLabel << ":\n";
    node->else_branch->accept(this);
} else {
    emitCondJumpIfFalse(node->condition, endLabel);
    node->then_branch->accept(this);
}
out_ << endLabel << ":\n";
```

`while` pone el label de inicio arriba y salta de vuelta al final; `for` es igual
pero con `init` antes, `update` antes del salto, y su propio scope para la variable
del init.

#### D. `break` / `continue`

Usan la pila `loop_labels_`, que guarda `{inicio, fin}` del loop actual (se hace
`push` al entrar, `pop` al salir):

```cpp
void CodeGenerator::visit(BreakStmt*)    { out_ << "    jmp " << loop_labels_.top().second << "\n"; }
void CodeGenerator::visit(ContinueStmt*) { out_ << "    jmp " << loop_labels_.top().first  << "\n"; }
```

Que sea una **pila** es lo que hace funcionar los loops anidados: `break` salta al
del tope, que es el más interno.

#### E. `delete`

`delete p` / `delete[] p` evalúan el puntero y llaman a `free`; el `[]` no cambia
nada (es un solo bloque de memoria).

---

### 9. Expresiones simples: literales e identificadores

Los literales son las hojas. Los inmediatos van directo a `%rax`
(`movq $valor, %rax`); el `char` se convierte a su código numérico con el helper
`charToCode`, que resuelve escapes como `'\n'`. Los `float` y `string` **no** caben
como inmediato, así que se registran en `.rodata` y se carga su dirección:

```cpp
void CodeGenerator::visit(FloatLitExpr* node) {
    out_ << "    movsd " << floatLabel(node->value) << "(%rip), %xmm0\n";
    cur_type_ = SemType{"float"};
}
void CodeGenerator::visit(StringLitExpr* node) {
    out_ << "    leaq " << strLabel(node->value) << "(%rip), %rax\n";  // un string es un puntero
    cur_type_ = SemType{"string"};
}
```

`visit(IdExpr)` lee una variable: si es array/struct (`is_array`) deja su
**dirección** (`leaq`), porque decae a puntero; si es escalar, carga su **valor**
con `emitLoad`.

---

### 10. Expresiones binarias: `visit(BinaryExpr)`

El nodo más cargado, con tres caminos.

#### A. Lógicos `&&` / `||` (cortocircuito)

Se tratan **primero y aparte**, porque el lado derecho no debe evaluarse si el
izquierdo ya decide el resultado (importa con punteros: `p != nullptr && p->x`). Se
implementa con saltos: se evalúa el izquierdo, y si ya determina el resultado se
salta sin tocar el derecho. El resultado se normaliza a 0/1.

#### B. El patrón de los demás

Para todo lo que no es lógico: evaluar izquierdo, **apilarlo**, evaluar derecho,
recuperar el izquierdo, combinar.

```cpp
node->left->accept(this);   SemType leftType = cur_type_;
emitPush(leftType);
node->right->accept(this);  SemType rightType = cur_type_;
```

Después se bifurca según haya o no un `float`.

**Ruta entera** (ambos enteros):

```cpp
out_ << "    movq %rax, %rcx\n";   // right → %rcx
emitPop(leftType, "%rax");          // left ← pila
switch (node->op) {
    case Add: out_ << "    addq %rcx, %rax\n"; break;
    case Sub: out_ << "    subq %rcx, %rax\n"; break;
    case Mul: out_ << "    imulq %rcx, %rax\n"; break;
    case Div: out_ << "    cqto\n    idivq %rcx\n"; break;             // cociente queda en %rax
    case Mod: out_ << "    cqto\n    idivq %rcx\n    movq %rdx, %rax\n"; break;  // resto en %rdx
    case /*comparaciones*/: out_ << "    cmpq %rcx, %rax\n"; emitSetccBool(op, false); return;
}
```

Y al final, el **tipo del resultado** aplica promoción numérica (`char + int → int`)
sin romper la aritmética de punteros:

```cpp
if (leftType.hasPointer())       cur_type_ = leftType;
else if (rightType.hasPointer()) cur_type_ = rightType;
else                             cur_type_ = SemType::promote(leftType, rightType);
```

**Ruta float** (alguno es `float`): ambos operandos se llevan a registros `%xmm`,
convirtiendo con `cvtsi2sdq` el que sea entero, y se opera con `addsd`/`subsd`/etc.,
o `ucomisd` para comparaciones.

---

### 11. Unarios e incremento/decremento

`visit(UnaryExpr)` cubre `-`, `!`, `*`, `&`, `++`, `--` prefijos: negación, negación
lógica (vía `emitCompareZero` + `sete`), dereferencia (`emitLoadIndirect` del tipo
apuntado), dirección (`emitLvalueAddr` + agregar `*` al tipo), y los `++`/`--` que
delegan en `emitIncDec`.

`emitIncDec` lo comparten prefijo y postfijo: calcula la dirección del lvalue una
sola vez, carga el valor viejo, le suma/resta 1, guarda el nuevo, y al final decide
qué deja en el registro:

```cpp
emitLvalueAddr(lvalue);              // dirección → %rax
out_ << "    movq %rax, %rcx\n";     // conservar la dirección
out_ << "    movq (%rcx), %rax\n";   // valor viejo
out_ << (inc ? "    leaq 1(%rax), %rdx\n" : "    leaq -1(%rax), %rdx\n");
out_ << "    movq %rdx, (%rcx)\n";   // guardar el nuevo
if (!postfix) out_ << "    movq %rdx, %rax\n";   // prefix devuelve el NUEVO; postfix el viejo
```

La única diferencia entre `++x` y `x++` es qué valor queda en `%rax` al final.
`visit(PostfixExpr)` llama a `emitIncDec` con `postfix=true`.

---

### 12. Asignación: `visit(AssignExpr)`

Dos casos. Si el destino es una variable simple, store directo por offset:

```cpp
if (auto* id = dynamic_cast<IdExpr*>(node->left)) {
    VarEntry* e = env_.lookup(id->name);
    node->right->accept(this);          // valor → %rax/%xmm0
    emitPromote(e->type);
    emitStore(e->type, e->offset);
    cur_type_ = e->type;
    return;
}
```

Si es un lvalue compuesto (`arr[i]`, `*p`, `s.x`), se calcula su **dirección**, se
guarda en la pila mientras se evalúa el lado derecho, y se hace un store indirecto:

```cpp
emitLvalueAddr(node->left);   // dirección → %rax
SemType t = cur_type_;
out_ << "    pushq %rax\n";    // proteger la dirección durante el RHS
node->right->accept(this);
emitPromote(t);
out_ << "    popq %rcx\n";
emitStoreIndirect(t, "%rcx");
cur_type_ = t;
```

Que la asignación deje `cur_type_` y el valor es lo que permite encadenar
`a = b = c`.

---

### 13. Llamadas: `visit(CallExpr)`

Primero despacha según quién es el *callee*:

```cpp
if (auto* id = dynamic_cast<IdExpr*>(node->callee)) {
    if (id->name == "print" || id->name == "println") { emitBuiltinPrint(node, ...); return; }
    if (frame_sizes_.count(id->name))                 { emitUserCall(node, id->name); return; }
}
```

#### A. `print` / `println` — `emitBuiltinPrint`

No son funciones del usuario: se traducen a una llamada a `printf` de la libc **por
cada argumento**, eligiendo el *format string* según el tipo real de cada uno
(`__fmt_int`, `__fmt_float`, `__fmt_char`, `__fmt_bool`, `__fmt_str`). `println`
agrega al final un `printf` del salto de línea. El tipo se lee de `cur_type_`, que
es justo para lo que sirve la convención de la introducción.

#### B. Funciones de usuario — `emitUserCall`

Implementa la convención de llamada: evalúa cada argumento (promoviéndolo al tipo del
parámetro), lo apila, y recuerda a qué banco (int/float) y posición le toca. Luego
los desapila **en orden inverso** hacia los registros de cada banco, hace `call`, y
restaura `cur_type_` con el retorno precalculado:

```cpp
for (size_t i = 0; i < n; ++i) {
    node->args[i]->accept(this);
    if (i < ptypes.size()) emitPromote(ptypes[i]);
    bool f = cur_type_.isFloat();
    regIdx[i] = f ? nFloat++ : nInt++;
    emitPush(cur_type_);
}
for (size_t k = n; k-- > 0; )   // inverso: la cima es el último argumento
    /* emitPop hacia INT_ARG_REGS[regIdx[k]] o FLOAT_ARG_REGS[...] */;
out_ << "    call " << name << "\n";
cur_type_ = func_rets_[name];
```

---

### 14. Memoria dinámica e indexado como rvalue

`new T[n]` reserva `n*8` bytes con `malloc`; `new Struct` usa `calloc(1, size)` para
dejar los campos en cero; ambos dejan el puntero en `%rax`. `delete` llama a `free`.

`visit(IndexExpr)` y `visit(MemberExpr)` como rvalue reusan `emitLvalueAddr` para
obtener la dirección y luego cargan el valor — salvo cuando es un sub-array, donde
la dirección ya es el resultado:

```cpp
void CodeGenerator::visit(IndexExpr* node) {
    emitLvalueAddr(node);
    SemType et = cur_type_;
    if (cur_array_decay_) { cur_type_ = et; return; }   // sub-array: no hacer load
    emitLoadIndirect(et);
    cur_type_ = et;
}
```

Aquí se ve para qué existe `cur_array_decay_`: sin esa guarda, `m[i]` (una fila de un
array 2D) intentaría leer memoria en lugar de quedarse con la dirección de la fila.

---

### Resumen del flujo

- **`gencode`** orquesta todo: primera pasada, genera el `.text` a un buffer, y
  emite `.data` / `.rodata` / `.text` / `.note.GNU-stack`.
- La **primera pasada** precalcula frames, retornos, params y layouts de struct, sin
  emitir nada.
- Cada **`visit`** traduce un nodo, apoyándose en los **helpers** (`emitLoad`,
  `emitPromote`, `emitLvalueAddr`, …) y respetando la convención: el valor queda en
  `%rax`/`%xmm0` y el tipo en `cur_type_`.
