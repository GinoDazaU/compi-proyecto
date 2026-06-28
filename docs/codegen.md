# Plan de Generación de Código — x86-64 AT&T

Basado en las convenciones del curso CS3402.

---

## 1. Arquitectura general

- `CodeGenerator` escribe a un `std::ostream` mientras recorre el AST — igual que el proyecto anterior.
- **Sin IR.** AST → assembly directo.
- El propio CodeGenerator precalcula el tamaño del frame de cada función en una primera pasada.
- El archivo generado se ensambla con `g++` que actúa como linker.

Estructura del `.s` generado:

```asm
.data
    ; format strings para printf

.text
    ; funciones una por una

.section .note.GNU-stack,"",@progbits
```

---

## 2. Registros clave

| Registro | Uso en nuestro compilador |
|---|---|
| `%rax` | Resultado de **toda expresión** (int, bool, char). Valor de retorno. |
| `%xmm0` | Resultado de **toda expresión float**. Valor de retorno float. |
| `%rcx` | Operando derecho en expresiones binarias. |
| `%rsi` | 2° arg de printf (valor a imprimir). |
| `%rdi` | 1° arg de printf (format string). |
| `%rbp` | Base pointer del frame actual. |
| `%rsp` | Stack pointer. |
| `%rdi,%rsi,%rdx,%rcx,%r8,%r9` | Args 1–6 de cualquier función (int/ptr). |
| `%xmm0–%xmm7` | Args float de funciones. |

---

## 3. Invariante central

> **Toda expresión deja su resultado en `%rax`** (int, bool, char) **o `%xmm0`** (float).  
> El resto del código asume que esto siempre se cumple al regresar de `e->accept(this)`.

---

## 4. Tipos → instrucciones

| Tipo | Tamaño stack | Store | Load | Registro |
|---|---|---|---|---|
| `int` | 8 bytes | `movq %rax, N(%rbp)` | `movq N(%rbp), %rax` | `%rax` |
| `float` | 8 bytes | `movsd %xmm0, N(%rbp)` | `movsd N(%rbp), %xmm0` | `%xmm0` |
| `bool` | 8 bytes | `movb %al, N(%rbp)` | `movb N(%rbp), %al` + `movzbq %al, %rax` | `%al` / `%rax` |
| `char` | 8 bytes | `movb %al, N(%rbp)` | `movb N(%rbp), %al` + `movzbq %al, %rax` | `%al` / `%rax` |

> Todos los tipos ocupan **8 bytes en stack** para simplicidad de alineación, aunque bool/char solo usen 1 byte de dato.

---

## 5. Expresiones binarias (int)

Convención del curso (Lab11):

```
1. evaluar left  → %rax
2. pushq %rax                  ; guardar left en pila
3. evaluar right → %rax
4. movq %rax, %rcx             ; right → %rcx
5. popq %rax                   ; left ← pila
6. <op> %rcx, %rax             ; resultado en %rax
```

Ejemplo `x + y`:
```asm
movq -8(%rbp), %rax      ; left (x) → %rax
pushq %rax
movq -16(%rbp), %rax     ; right (y) → %rax
movq %rax, %rcx
popq %rax
addq %rcx, %rax          ; resultado en %rax
```

### Operaciones aritméticas

| Op | Instrucción |
|---|---|
| `+` | `addq %rcx, %rax` |
| `-` | `subq %rcx, %rax` |
| `*` | `imulq %rcx, %rax` |
| `/` | `cqto` + `idivq %rcx` (cociente en `%rax`) |
| `%` | `cqto` + `idivq %rcx` (resto en `%rdx`, mover a `%rax`) |

### Comparaciones (resultado bool en %rax)

```asm
cmpq %rcx, %rax
movl $0, %eax
set<cc> %al          ; setl, setle, setg, setge, sete, setne
movzbq %al, %rax     ; zero-extend a 64 bits
```

| Op | `set<cc>` |
|---|---|
| `<`  | `setl`  |
| `<=` | `setle` |
| `>`  | `setg`  |
| `>=` | `setge` |
| `==` | `sete`  |
| `!=` | `setne` |

### Lógicos (&&, ||)

`&&` y `||` se generan con cortocircuito: se evalúa el izquierdo, se normaliza a
`0/1`, y si ya determina el resultado se salta sin evaluar el derecho. Cada
operando se normaliza con `cmpq $0` + `setne` (o `ucomisd` contra 0 si es
`float`).

```asm
; a && b
<eval a> → %rax
cmpq $0, %rax
je   __logic_short_N      ; a falso → resultado 0, no se evalúa b
<eval b> → %rax
cmpq $0, %rax
movl $0, %eax
setne %al                 ; resultado = (b != 0)
movzbq %al, %rax
jmp  __logic_end_N
__logic_short_N:
movq $0, %rax
__logic_end_N:

; a || b : idéntico pero `jne __logic_short_N` y el corto carga $1
```

### Expresiones binarias float

```
1. evaluar left  → %xmm0
2. subq $8, %rsp  +  movsd %xmm0, (%rsp)   ; "push" float
3. evaluar right → %xmm0
4. movsd %xmm0, %xmm1                       ; right → %xmm1
5. movsd (%rsp), %xmm0  +  addq $8, %rsp   ; "pop" left → %xmm0
6. <op>sd %xmm1, %xmm0                     ; resultado en %xmm0
```

| Op | Instrucción |
|---|---|
| `+` | `addsd %xmm1, %xmm0` |
| `-` | `subsd %xmm1, %xmm0` |
| `*` | `mulsd %xmm1, %xmm0` |
| `/` | `divsd %xmm1, %xmm0` |

Comparaciones float: `ucomisd %xmm1, %xmm0` + `set<cc>` igual que int.

---

## 6. Variables locales

### Asignación de offsets

- `offset_` empieza en `-8` al inicio de cada función.
- Cada variable nueva: `offset_actual = offset_`; `offset_ -= 8`.
- El environment guarda `{SemType, int}` — tipo y offset.

```cpp
// Pseudocódigo de visit(VarDeclStmt)
int var_offset = offset_;
env.declare(node->name, {type, var_offset});
offset_ -= 8;
// emitir store si hay inicializador
```

### Acceso a variable (IdExpr)

```asm
movq N(%rbp), %rax     ; int
movsd N(%rbp), %xmm0   ; float
movb N(%rbp), %al      ; bool/char
movzbq %al, %rax
```

---

## 7. Prólogo y epílogo

### Precálculo del frame

El CodeGenerator acumula el tamaño del frame de cada función en un mapa
`frame_sizes_` durante su `firstPass` (recorre el cuerpo contando declaraciones
locales y parámetros, 8 bytes cada uno):

```cpp
// firstPass: por cada FuncDecl
frame_sizes_[f->name] = frameSize(f);  // params + locales, ×8, redondeado a 16
```

El frame se redondea al múltiplo de 16 más cercano (requisito del ABI):

```cpp
int frame = frame_sizes_[name];
if (frame % 16 != 0) frame += (16 - frame % 16);
```

### Prólogo

```asm
.globl funcname
funcname:
    pushq %rbp
    movq %rsp, %rbp
    subq $N, %rsp        ; N = frame size calculado
```

### Epílogo

```asm
.end_funcname:
    leave                ; equivale a: movq %rbp, %rsp + popq %rbp
    ret
```

### Return

```asm
; resultado ya en %rax (o %xmm0 para float)
jmp .end_funcname
```

---

## 8. Llamadas a función

### Preparar argumentos

Evaluar cada arg en orden, guardarlo en la pila, luego moverlos a los registros:

```asm
; para cada arg i:
<eval arg i>              ; resultado en %rax o %xmm0
pushq %rax               ; int/bool/char
; (o: subq $8,%rsp + movsd %xmm0,(%rsp)  para float)
```

Luego, en orden inverso, sacar de la pila a los registros de argumento (cada
banco lleva su propio índice):
- int args: `%rdi, %rsi, %rdx, %rcx, %r8, %r9`
- float args: `%xmm0, %xmm1, %xmm2, ..., %xmm7`

```asm
call funcname
; resultado en %rax (int) o %xmm0 (float)
```

> Solo se pasan args por registros: hasta 6 int y 8 float. El semántico rechaza
> funciones que excedan ese límite (ver `semantic_rules.md §2.2`). El tipo de
> retorno de cada función se precalcula en `firstPass` (`func_rets_`) para dejar
> `cur_type_` correcto tras la llamada.

---

## 9. Control de flujo

**Invariante:** toda condición evalúa a 0 (falso) o 1 (verdadero) en `%rax`. El patrón siempre es:

```asm
cmpq $0, %rax
je __label_false_N     ; salta si es falso (rax == 0)
```

### Labels

Contador global `label_counter_` (int) que incrementa en cada estructura de control.  
Prefijo `__` para evitar colisiones con nombres de usuario.

### If / else

```asm
; condición → %rax
cmpq $0, %rax
je __else_N
; then body
jmp __endif_N
__else_N:
; else body (si existe, sino vacío)
__endif_N:
```

### While

```asm
__while_N:
    ; condición → %rax
    cmpq $0, %rax
    je __endwhile_N
    ; body
    jmp __while_N
__endwhile_N:
```

### For clásico

```asm
; init
__for_N:
    ; condición → %rax
    cmpq $0, %rax
    je __endfor_N
    ; body
    ; update
    jmp __for_N
__endfor_N:
```

### Break / Continue

Un stack `loop_labels_` guarda el label del loop actual.  
- `break`  → `jmp __endXXX_N`  
- `continue` → `jmp __XXX_N`

---

## 10. print / println

### Format strings en .data

```asm
.data
__fmt_int:    .string "%ld"
__fmt_float:  .string "%lf"
__fmt_char:   .string "%c"
__fmt_bool:   .string "%d"
__fmt_str:    .string "%s"
__fmt_nl:     .string "\n"
```

### Emitir una llamada printf por argumento

**int / bool / char:**
```asm
movq %rax, %rsi
leaq __fmt_int(%rip), %rdi
movl $0, %eax              ; 0 registros XMM usados
call printf@PLT
```

**float:**
```asm
; %xmm0 ya tiene el valor
leaq __fmt_float(%rip), %rdi
movl $1, %eax              ; 1 registro XMM usado
call printf@PLT
```

**string:**
```asm
movq %rax, %rsi            ; puntero al string
leaq __fmt_str(%rip), %rdi
movl $0, %eax
call printf@PLT
```

**println:** igual que print, pero al final emite:
```asm
leaq __fmt_nl(%rip), %rdi
movl $0, %eax
call printf@PLT
```

---

## 11. Structs

- Cada struct tiene un `CodegenStructInfo` con `offsets` (nombre→offset), `types` (nombre→`SemType`) y `size`. Se calcula en `firstPass` (`buildStructInfo`).
- Cada miembro ocupa un slot de 8 bytes, en orden de declaración: offsets **positivos** desde la base del struct (`0, 8, 16...`).
- Una variable struct reserva `size` bytes inline en el frame y decae a su dirección base: su "valor" es esa dirección (`leaq`).
- Acceso `s.x` → dirección base del struct `+ offset_de_x`. Con `p->x`, la dirección base es el valor del puntero.
- Paso por puntero (`Struct*`): se pasa esa dirección base en el registro de argumento.
- `new Struct` reserva el objeto con `calloc(1, size)` (campos en cero) y devuelve el puntero.

---

## 12. Arrays

Todo elemento ocupa un slot de 8 bytes (igual que las variables locales).

**Estático** (`int arr[5]`):
- Reserva `n * 8` bytes inline en el frame y decae a puntero (`T*`); la variable guarda la dirección de `arr[0]`.
- `arr[i]` → `base + i*8`: se evalúa el índice, se multiplica por 8 y se suma a la dirección base.

```asm
; arr[i] (dirección del elemento):
<eval i> → %rax
imulq $8, %rax
leaq <base>(%rbp), %rcx   ; dirección de arr[0]
addq %rcx, %rax           ; dirección de arr[i]
movq (%rax), %rax         ; load del elemento
```

**Multidimensional** (`int m[d0][d1]`):
- Almacenamiento plano row-major: reserva `d0*d1*...*8` bytes y decae a un nivel de puntero por dimensión (`int**` para 2D).
- `m[i][j]` se direcciona con un índice lineal por Horner (`(i*d1 + j)*8 + base`). Las dimensiones se guardan en `VarEntry::dims`.

**String** (`s[i]`):
- Un `string` apunta a bytes empaquetados: stride **1** (no 8) y elemento `char`.

**Puntero** (`p[i]`, p.ej. de `new`):
- Se carga el valor del puntero base y se indexa con stride 8.

**Dinámico** (`new int[n]`):
```asm
; n ya en %rax
imulq $8, %rax
movq %rax, %rdi
call malloc@PLT            ; resultado (puntero) en %rax
```

**`delete[]`:**
```asm
; puntero en %rax
movq %rax, %rdi
call free@PLT
```

---

## 13. Datos estáticos (string literals)

Las cadenas literales van en `.rodata`:

```asm
.section .rodata
__str_0:  .string "hola mundo"
```

La dirección se carga con `leaq __str_0(%rip), %rax`.

Un contador `str_counter_` genera labels únicos `__str_0`, `__str_1`, etc.

---

## 14. Sección final obligatoria

Todo archivo `.s` generado debe terminar con:

```asm
.section .note.GNU-stack,"",@progbits
```

Esto le dice al linker que el stack no es ejecutable (requerido en Linux moderno).

---

## 15. Estructura del CodeGenerator

```cpp
class CodeGenerator : public Visitor {
    std::ostream&  out_;
    SymbolTable<VarEntry> env_;  // VarEntry = { SemType type; int offset; bool is_array; vector<int> dims; }
    std::unordered_map<std::string, int>               frame_sizes_;
    std::unordered_map<std::string, CodegenStructInfo> structs_;  // offsets + types + size

    int  offset_        = -8;    // offset de la próxima var local
    int  label_counter_ = 0;     // para labels únicos
    int  str_counter_   = 0;     // para string literals
    int  float_counter_ = 0;     // para float literals
    std::string current_func_;

    SemType cur_type_;           // tipo de la última expresión evaluada (ver abajo)

    std::stack<std::pair<std::string, std::string>> loop_labels_; // break/continue

    // Constantes recolectadas durante la emisión, volcadas luego a .rodata
    std::vector<std::pair<std::string, std::string>> string_literals_;
    std::vector<std::pair<std::string, double>>      float_literals_;
};
```

El CodeGenerator es **autosuficiente**: calcula `frame_sizes_` y `structs_` en su propia
primera pasada y no depende de lo que dejó el TypeChecker (el AST no se anota).

### Tipo de cada expresión: `cur_type_`

Como nuestro `Visitor::visit(...)` devuelve `void`, una expresión no puede *retornar*
su tipo. En su lugar, cada `visit` de expresión deja su `SemType` en el miembro
`cur_type_` justo después de dejar el valor en `%rax`/`%xmm0`, y el nodo padre lo
consulta. Es el equivalente a "devolver el tipo desde `accept()`", pero vía miembro.
Esto es lo que permite, por ejemplo, que `print`/`println` elijan el formato y el
registro (`%rax` vs `%xmm0`) según el tipo real de cada argumento.

Los `float` y `string` literales no caben como inmediato: se registran en
`float_literals_`/`string_literals_` (con dedup) y se emiten en `.rodata`. Por eso el
`.text` se genera primero a un buffer y las secciones de datos se escriben después,
cuando ya se conocen todas las constantes.

---

## 16. Ejemplo completo: fibonacci

Entrada:
```cpp
int fibonacci(int n) {
    if (n <= 1) { return n; }
    return fibonacci(n - 1) + fibonacci(n - 2);
}
```

Assembly esperado (simplificado):
```asm
.globl fibonacci
fibonacci:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp          ; frame: 1 param (n) + margen

    movq %rdi, -8(%rbp)     ; guardar param n

    ; if (n <= 1)
    movq -8(%rbp), %rax     ; left = n
    pushq %rax
    movq $1, %rax           ; right = 1
    movq %rax, %rcx
    popq %rax
    cmpq %rcx, %rax
    movl $0, %eax
    setle %al
    movzbq %al, %rax
    cmpq $0, %rax
    je __endif_0
    movq -8(%rbp), %rax     ; return n
    jmp .end_fibonacci
    __endif_0:

    ; fibonacci(n-1) + fibonacci(n-2)
    ...
    call fibonacci
    pushq %rax
    ...
    call fibonacci
    movq %rax, %rcx
    popq %rax
    addq %rcx, %rax
    jmp .end_fibonacci

.end_fibonacci:
    leave
    ret
```
