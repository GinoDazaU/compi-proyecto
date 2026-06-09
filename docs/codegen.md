# Plan de Generación de Código — x86-64 AT&T

Basado en `docs/refs/guia_assembly.pdf` y las convenciones del curso CS3402.

---

## 1. Arquitectura general

- `GenCodeVisitor` escribe a un `std::ostream` mientras recorre el AST — igual que el proyecto anterior.
- **Sin IR.** AST → assembly directo.
- El type checker precalcula el tamaño del frame de cada función.
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

```asm
; && : and bit a bit de los bytes bajos
and %cl, %al
movzbq %al, %rax

; || : or bit a bit
or %cl, %al
movzbq %al, %rax
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

El type checker acumula el tamaño del frame de cada función en un mapa `frame_sizes_`:

```cpp
// en visit(VarDeclStmt) del TypeChecker:
frame_sizes_[current_func_] += 8;  // todos los tipos: 8 bytes en stack
// también los parámetros al entrar a FuncDecl
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

Luego, en orden inverso, sacar de la pila a los registros de argumento:
- int args: `%rdi, %rsi, %rdx, %rcx, %r8, %r9`
- float args: `%xmm0, %xmm1, %xmm2, ..., %xmm7`

```asm
call funcname
; resultado en %rax (int) o %xmm0 (float)
```

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

- Cada struct tiene un `StructInfo` con `offsets` (mapa nombre→offset) y `size`.
- Los miembros se almacenan en stack de forma contigua (offsets negativos desde la base del struct).
- Acceso `s.x` → `base_offset + offset_de_x` desde `%rbp`.
- Paso por referencia (`&`): se pasa la dirección (`%rbp + offset`) en el registro de argumento.

---

## 12. Arrays

**Estático** (`int arr[5]`):
- Reservar `n * 8` bytes en el frame (en lugar de 8).
- `arr[i]` → `base_offset + i*8` desde `%rbp`.
- El índice `i` se evalúa, multiplica por 8, y se usa en modo de direccionamiento indexado.

```asm
; arr[i] load:
movq -40(%rbp), %rax      ; base de arr
<eval i> → %rcx
imulq $8, %rcx
subq %rcx, %rax           ; ajuste de dirección (offsets negativos)
movq (%rax), %rax
```

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

## 15. Estructura del GenCodeVisitor

```cpp
class GenCodeVisitor : public Visitor {
    std::ostream&  out;
    SymbolTable<std::pair<SemType, int>> env_;  // tipo + offset
    std::unordered_map<std::string, int>  frame_sizes_;   // del type checker
    std::unordered_map<std::string, int>  func_ret_size_; // tamaño retorno struct
    std::unordered_map<std::string, StructInfo> structs_; // del type checker

    int  offset_       = -8;     // offset de la próxima var local
    int  label_counter_= 0;      // para labels únicos
    int  str_counter_  = 0;      // para string literals
    std::stack<std::string> loop_labels_; // para break/continue
    std::string current_func_;
};
```

El GenCodeVisitor **recibe** `frame_sizes_` y `structs_` del TypeChecker (o los recalcula en una primera pasada propia).

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
