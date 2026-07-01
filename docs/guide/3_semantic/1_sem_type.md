# El Tipo Resuelto (SemType)

Mientras el parser construye el AST, cada tipo escrito por el usuario queda
guardado como un `TypeNode`: la forma **sintáctica** del tipo (`int`, `MiStruct*`,
`auto`). Eso sirve para *recordar* lo que se escribió, pero no para *razonar*: no
sabe comparar dos tipos, ni si un `int` cabe en un `float`, ni qué queda al
desreferenciar un puntero.

El análisis semántico necesita una representación **limpia y comparable**, y esa
es `SemType`. Está definida en `sem_type.h` (la estructura y las consultas
sencillas, que son `inline`) y `sem_type.cpp` (la lógica de comparación y
promoción). Vamos a recorrerla.

---

### 1. La estructura en sem_type.h

```cpp
struct SemType {
    std::string         base;   // "int", "float", "MiStruct", ...
    std::vector<PtrMod> mods;   // modificadores en orden: *
};
```

Un `SemType` es **un nombre base + una lista de modificadores de puntero**. `int`
es `{base:"int", mods:[]}`; `int**` es `{base:"int", mods:[Pointer, Pointer]}`.

Fíjate en lo que **no** tiene, comparado con el `TypeNode` del AST: `is_auto`.
Para cuando existe un `SemType`, `auto` ya fue reemplazado por el tipo inferido.
El puente entre ambos mundos es `fromTypeNode`, que simplemente copia `base` y
`mods`:

```cpp
SemType SemType::fromTypeNode(const TypeNode* node) {
    if (!node) return SemType{"void"};
    SemType t;
    t.base = node->base;
    t.mods = node->mods;
    return t;
}
```

---

### 2. Rango numérico y promoción

El orden de "tamaño" de los tipos numéricos se define en **un único lugar**, una
función estática en `sem_type.cpp`:

```cpp
static int numericRank(const std::string& base) {
    if (base == "bool")   return 0;
    if (base == "char")   return 1;
    if (base == "int")    return 2;
    if (base == "float")  return 3;
    return -1;   // no es numérico
}
```

Sobre ese rango se construye `promote`, que dado dos tipos numéricos devuelve el
de **mayor rango**. Es lo que hace que `int + float` sea `float` o que
`char + int` sea `int`:

```cpp
SemType SemType::promote(const SemType& a, const SemType& b) {
    int ra = numericRank(a.base);
    int rb = numericRank(b.base);
    if (ra < 0 || rb < 0)
        throw std::runtime_error("promote: non-numeric types");
    return ra >= rb ? a : b;
}
```

Dos detalles importantes: `promote` **lanza** si algún operando no es numérico
(quien la llama debe filtrar antes con `isArithmetic`), y mira **solo `base`**,
ignorando `mods` — por eso no se debe usar con punteros.

---

### 3. Compatibilidad: `accepts`

`a.accepts(b)` responde la pregunta central de todo lenguaje con tipos: *¿puedo
poner un valor de tipo `b` donde se espera un `a`?* (asignar, pasar como
argumento, `return`). Su cuerpo es corto y vale la pena leerlo entero:

```cpp
bool SemType::accepts(const SemType& other) const {
    if (*this == other) return true;                       // 1. mismo tipo exacto

    if (hasPointer() || other.hasPointer()) return false;  // 2. punteros: solo idénticos

    int myRank    = numericRank(base);                     // 3. promoción numérica
    int otherRank = numericRank(other.base);
    if (myRank >= 0 && otherRank >= 0) return otherRank <= myRank;

    return false;
}
```

Las tres ramas son las tres reglas del lenguaje:

1. Tipos idénticos siempre son compatibles.
2. Los punteros no aceptan coerción: o son el mismo puntero, o nada.
3. Entre numéricos, `other` se acepta si su rango es **≤** al nuestro: un `float`
   acepta un `int` (`2 ≤ 3`), pero un `int` **no** acepta un `float`.

```cpp
SemType{"float"}.accepts(SemType{"int"})   // true  (int → float)
SemType{"int"}.accepts(SemType{"float"})   // false (perdería precisión)
```

---

### 4. Consultas, punteros y `deref`

El header trae un montón de consultas `inline` que el resto del compilador usa a
cada paso. La pieza clave es `hasPointer`, y a partir de ella se definen las demás:

```cpp
bool hasPointer()  const { return !mods.empty() && mods.back() == PtrMod::Pointer; }
bool isNumeric()   const { return !hasPointer() && (base=="int"||base=="float"); }
bool isFloat()     const { return !hasPointer() && base=="float"; }

// bool/char se cargan y guardan en 1 byte; el codegen lo consulta seguido
bool isByteSized() const { return (base=="bool"||base=="char") && !hasPointer(); }
```

Y `deref`, que quita un nivel de puntero (`int**` → `int*`):

```cpp
SemType deref() const {
    SemType t = *this;
    if (!t.mods.empty()) t.mods.pop_back();
    return t;
}
```

Esto es lo que permite **no modelar los arrays aparte**: un array decae a puntero
(un `*` por dimensión), así `int arr[5]` se trata como `int*` y `arr[i]` se tipa
con `deref()`. El `string` es la excepción: `s[i]` da `char` directamente, no por
`deref` (un `string` no lleva `mods`).

---

### En contexto

`SemType` es la moneda común del semántico: el TypeChecker calcula uno por cada
expresión. Y no se queda ahí — el mismo `SemType` reaparece en el generador de
código, que lo consulta para elegir registro (`%rax` vs `%xmm0`), instrucción de
load/store y formato de impresión. Las reglas de compatibilidad que `accepts` y
`promote` implementan están listadas en `semantic_rules.md`.
