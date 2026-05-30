# Compilador de C++ → x86-64

Compilador de un subconjunto de C++ que genera código ensamblador x86-64 (AT&T syntax).
Escrito desde cero en C++. Incluye frontend web para visualización (bonus del proyecto).

Referencia del ciclo anterior: `~/Desktop/rust-compiler` — compilador de Rust→x86 con la misma arquitectura, útil para consultar decisiones de diseño en codegen y manejo de structs.

---

## Estructura del proyecto

```
compi-proyecto/
├── compiler/
│   ├── src/
│   │   ├── lexer/       # Tokenización
│   │   ├── parser/      # Parser recursivo descendente + AST
│   │   ├── semantic/    # Type checker y manejo de scope
│   │   ├── codegen/     # Emisión de assembly x86-64
│   │   └── optimizer/   # Optimizaciones sobre el código generado
│   ├── tests/           # Archivos .cpp de prueba por fase
│   └── CMakeLists.txt
├── backend/             # API REST en Python que expone el compilador
├── frontend/            # App web React/Vite
├── benchmarks/          # Comparación con GCC y Clang
└── docs/
```

---

## Fases del compilador

### 1. Lexer (`lexer/`)
Convierte el texto fuente en tokens. Maneja:
- Keywords, identificadores, literales (int, float, bool, char, string)
- Operadores, puntuación
- Comentarios `//` y `/* */` (incluyendo anidados)
- Reporte de errores léxicos con línea y columna

### 2. Parser + AST (`parser/`)
Parser recursivo descendente. Produce un AST tipado.
- Nodos separados por categoría: expresiones, statements, declaraciones
- Patrón Visitor para recorrer el árbol
- Gramática documentada en `docs/grammar.md` (a crear)

### 3. Semántico (`semantic/`)
Dos pasadas sobre el AST:
- **Primera pasada:** recolecta declaraciones de structs y funciones
- **Segunda pasada (CheckVisitor):** verifica tipos, scope, uso correcto de variables, retornos

Tabla de símbolos con soporte de scopes anidados.

El compilador debe reportar errores claros en las tres fases: léxico (token inválido), sintáctico (estructura inesperada) y semántico (tipo incorrecto, variable no declarada, etc.), con número de línea y columna.

### 4. Generación de código (`codegen/`)
Emite assembly x86-64 AT&T syntax, enlazable con `gcc`.
- Convención de llamada System V AMD64 (Linux)
- Stack frame estándar: `pushq %rbp / movq %rsp, %rbp`
- Variables locales en offsets negativos desde `%rbp`
- Paso de argumentos: `%rdi, %rsi, %rdx, %rcx, %r8, %r9`, resto en stack
- Structs: se pasan por referencia (dirección en registro)
- Soporte de `printf` para output

### 5. Optimizador (`optimizer/`)
Optimizaciones básicas sobre el AST o sobre el código intermedio:
- Constant folding
- Dead code elimination
- (Posiblemente) eliminación de subexpresiones comunes

---

## Subconjunto de C++ a soportar

### Básico
- Tipos: `int`, `long`, `float`, `bool`, `char`
- Variables con scope, `const`
- Funciones con parámetros y valor de retorno
- Control: `if/else`, `while`, `for`, `break`, `continue`, `return`
- Operadores aritméticos, lógicos, relacionales, de asignación (`+=`, `-=`, etc.)

### Intermedio
- `struct` con atributos y paso por referencia (`&`)
- Arrays estáticos
- Strings (`std::string` básico o strings de C)
- Punteros y aritmética de punteros
- Memoria dinámica: `new` / `delete`

### Avanzado
- Templates simples (`template<typename T>`)
- `auto` (inferencia de tipos)
- Inferencia, conversión y promoción automática de tipos (`static_cast`, conversiones implícitas `int`→`float`, etc.)
- Lambdas básicas
- Arrays multidimensionales

---

## Frontend web (bonus)

App React/Vite conectada a un servidor Python (FastAPI o Flask) que:
1. Expone el compilador como API REST
2. Muestra un editor de código con syntax highlighting
3. Visualiza el AST generado
4. Muestra el assembly x86-64 generado
5. Ejecuta o simula el programa compilado y muestra el output

---

## Benchmarks

Comparar el compilador propio contra GCC, Clang/LLVM, MSVC, Rust compiler y Go compiler en:
- Tiempo de compilación
- Tamaño del binario generado
- Velocidad de ejecución (programas de prueba: fibonacci, ordenamiento, etc.)

Los resultados se documentan con tablas, gráficos y discusión técnica en el reporte final.

---

## Decisiones de diseño

- El compilador se implementa en **C++17**
- Se usa **CMake** para el build system
- El assembly generado se ensambla con `gcc` (actúa como linker)
- Nomenclatura de labels internos: prefijo `__` para evitar colisiones con nombres de funciones del usuario
- Los tipos numéricos usan siempre registros de 64 bits (`%rax`, etc.) para simplificar el codegen
