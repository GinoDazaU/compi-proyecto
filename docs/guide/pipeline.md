# El Pipeline del Compilador

Este documento es el cierre de la guía: en vez de mirar una fase, mira **cómo se
conectan todas**. Un compilador es una cadena de transformaciones que convierte
texto fuente en código ejecutable; cada fase consume lo que produjo la anterior y
le pasa su resultado a la siguiente. Cada fase tiene su propio documento en esta
guía; aquí vemos el panorama completo y los puntos donde se enganchan.

---

## 1. La cadena de transformaciones

```
código fuente (.txt)
      │
      ▼  Lexer                  texto → secuencia de tokens
   tokens
      │
      ▼  Parser                 tokens → árbol (AST)
    AST
      │
      ▼  Semántico (TypeChecker) verifica tipos y scopes; el AST queda validado
 AST validado
      │
      ▼  Optimizador (opcional)  reescribe el AST (--opt)        [aún no implementado]
 AST optimizado
      │
      ▼  CodeGenerator           AST → ensamblador x86-64
  archivo .s
      │
      ▼  g++                     ensambla y enlaza
  ejecutable
```

Cada flecha es un cambio de representación: el programa va perdiendo "forma de
texto" y ganando "forma de máquina". Lo interesante es que las representaciones
intermedias (tokens, AST) son **datos** que se pueden inspeccionar — y de hecho el
compilador permite detenerse en cualquiera de ellas (sección 4).

---

## 2. Fase por fase, en resumen

**Lexer** — Lee el texto carácter por carácter y lo agrupa en *tokens*: las
unidades mínimas con significado (`int`, `x`, `=`, `42`, `;`). Quita espacios y
comentarios. Su salida es una lista plana de tokens, cada uno con su tipo, su texto
y su posición (línea/columna).

**Parser + AST** — Toma los tokens y, siguiendo la gramática, construye el **Árbol
de Sintaxis Abstracta**: una estructura jerárquica donde `2 + 3 * 4` ya refleja la
precedencia correcta. Es descenso recursivo; cada regla de la gramática es una
función. El AST es la representación central sobre la que trabajan todas las fases
siguientes, y todas lo recorren con el **patrón Visitor**.

**Semántico (TypeChecker)** — El AST ya es correcto de *forma*, pero puede tener
errores de *significado*: variables no declaradas, tipos incompatibles, argumentos
de más. El semántico recorre el árbol verificando tipos y alcances. No transforma el
AST; lo aprueba (o lo rechaza con un error).

**Optimizador** — *(Pendiente, esqueleto.)* Correría **sobre el AST, antes del
codegen** (solo con el flag `--opt`), aplicando mejoras como *constant folding* y
*dead code elimination*. Recibiría el AST validado y devolvería otro AST
equivalente pero más eficiente. Su documento queda como TODO.

**CodeGenerator** — La última fase: traduce el AST a ensamblador x86-64, que `g++`
ensambla y enlaza. Es la fase que más sabe de la máquina (registros, stack, la
convención de llamada).

---

## 3. Cómo se conectan: `main.cpp`

La orquestación vive en `main.cpp`, y se lee como la cadena de arriba, una fase tras
otra:

```cpp
// Fase 1: Léxico
Lexer lexer(source);
std::vector<Token> tokens = lexer.tokenize();

// Fase 2: Parser + AST
Parser parser(tokens);
Program* program = parser.parse();

// Fase 3: Semántico
TypeChecker checker;
checker.check(program);

// Fase 3.5: Optimización opcional sobre el AST
if (opt) optimizer::optimize(program);

// Fase 4: Generación de código
CodeGenerator gen(std::cout);
gen.gencode(program);
```

La salida de cada paso es la entrada del siguiente: `tokens` alimenta al parser,
`program` (el AST) alimenta al semántico, al optimizador y al codegen. Es
literalmente la guía leída en orden.

---

## 4. Los puntos de salida (flags)

El compilador no siempre llega hasta el final: según el flag, se detiene en una fase
y vuelca esa representación intermedia. Esto es clave para depurar y para alimentar
al frontend web.

| Flag | Hasta dónde llega | Qué imprime |
|---|---|---|
| `--tokens` | Lexer | la lista de tokens |
| `--ast` | Parser + Semántico | el AST en JSON |
| `--json` | Parser + Semántico | tokens **y** AST en JSON (para el frontend) |
| `--asm` | todo | el ensamblador x86-64 |
| `--opt` | (modificador) | activa el optimizador antes del codegen |

Que cada fase produzca un dato inspeccionable es lo que hace posible esto: poder
"ver los tokens" o "ver el AST" sin compilar todo.

---

## 5. Manejo de errores transversal

Las tres fases de análisis son **fail-fast**: ante el primer problema lanzan una
excepción con línea y columna, y la cadena se detiene. Hay un tipo de error por
fase —léxico (carácter inválido), sintáctico (`ParseError`) y semántico
(`SemanticError`)— y `main.cpp` los captura para imprimir un mensaje claro, o
serializarlos a JSON cuando se usa `--json`:

```cpp
} catch (const ParseError& e) {
    std::cerr << "syntax error at " << e.line << ":" << e.col << ": " << e.what() << "\n";
} catch (const SemanticError& e) {
    std::cerr << "semantic error at " << e.line << ":" << e.col << ": " << e.what() << "\n";
}
```

Por eso un programa con un error nunca llega al codegen: la fase que lo detecta corta
la cadena antes.

---

## 6. Cómo construir y correr

El orquestador de build y pruebas es `build.py`:

```bash
python3 build.py build                 # compila el compilador
python3 build.py test                  # corre toda la batería de pruebas
python3 build.py run --asm prog.txt    # genera el ensamblador de un programa
```

Para producir un ejecutable de verdad, el `.s` que sale del codegen se ensambla con
`g++`, que actúa como ensamblador y linker — el último eslabón de la cadena, fuera
ya de nuestro compilador.

---

## En contexto

Si leíste la guía en orden —lexer, parser, semántico, codegen— este pipeline es el
hilo que los une: cada documento explica una fase por dentro, y esta página explica
cómo se pasan el trabajo de una a otra hasta llegar al ejecutable.
