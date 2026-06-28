# Patrón Visitor en el Compilador

El patrón **Visitor** (Visitante) es un patrón de diseño de comportamiento que permite separar algoritmos y lógica de la estructura de objetos sobre la que operan. En este compilador, se utiliza como el mecanismo central para recorrer el **Árbol de Sintaxis Abstracta (AST)** sin modificar las clases de los nodos cada vez que se agrega una nueva fase (como chequeo de tipos o generación de código).

---

## 1. ¿Por qué usamos el patrón Visitor?

Los nodos del AST representan la estructura gramatical del programa. Si quisiéramos implementar la lógica de las diferentes fases directamente dentro de estos nodos, tendríamos clases sumamente sobrecargadas con responsabilidades de:
* Formateo e impresión (para depurar).
* Verificación de tipos semánticos y reglas de alcance.
* Generación de código ensamblador x86-64.

Cualquier cambio en la lógica de generación de código requeriría modificar decenas de archivos de definición de nodos del AST. 

El patrón Visitor resuelve esto:
1. Mantiene las clases de los nodos del AST muy simples y enfocadas únicamente en almacenar los datos del nodo.
2. Agrupa toda la lógica de una fase específica en una sola clase dedicada (como `TypeChecker`).

---

## 2. Funcionamiento: El Mecanismo de Double Dispatch

El patrón Visitor funciona mediante un doble despacho (double dispatch). El flujo es el siguiente:

1. El cliente (el compilador) llama a `nodo->accept(visitor)`.
2. El método `accept` (que está implementado en cada clase concreta del AST) redirige la llamada al visitor ejecutando `visitor->visit(this)`.
3. C++ resuelve automáticamente a qué sobrecarga de `visit` llamar basándose en el tipo exacto del puntero `this`.

> **Regla de simetría:** Todo nodo del AST debe implementar el método `accept(Visitor*)`, y la interfaz `Visitor` debe implementar un método `visit` para cada nodo. Ambos van siempre de la mano; si añades un nuevo tipo de nodo, debes asegurarte de que implemente `accept` y que el visitor lo visite.


### Ejemplo en Código:

En la clase base de expresiones (`Expr`):
```cpp
class Expr {
public:
    virtual ~Expr() = default;
    virtual void accept(Visitor* v) = 0; // Método de despacho
};
```

En un nodo concreto del AST como `BinaryExpr`:
```cpp
class BinaryExpr : public Expr {
public:
    Expr*    left;
    BinaryOp op;
    Expr*    right;

    void accept(Visitor* v) override {
        v->visit(this); // Llama a la sobrecarga correspondiente en el Visitor
    }
};
```

---

## 3. La Interfaz Base: `visitor.h`

La clase base `Visitor` declara un método virtual puro `visit` para cada una de las clases concretas del AST:

```cpp
class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit(Program* node) = 0;

    // Expresiones
    virtual void visit(IntLitExpr* node) = 0;
    virtual void visit(FloatLitExpr* node) = 0;
    virtual void visit(IdExpr* node) = 0;
    virtual void visit(BinaryExpr* node) = 0;
    // ... resto de expresiones ...

    // Sentencias
    virtual void visit(Block* node) = 0;
    virtual void visit(VarDeclStmt* node) = 0;
    virtual void visit(IfStmt* node) = 0;
    // ... resto de sentencias ...
};
```

Cualquier clase que quiera realizar un recorrido por el AST debe heredar de esta interfaz e implementar todos sus métodos.

---

## 4. Ejemplo Concreto: Un Evaluador de Expresiones Aritméticas

Para entender cómo se implementa un visitor, supongamos que queremos construir un evaluador que recorra un AST de expresiones simples (`BinaryExpr` con operadores básicos e `IntLitExpr`) y devuelva su valor numérico.

### Implementación del Visitor:

```cpp
#include "visitor.h"
#include "ast.h"
#include <iostream>

class EvaluatorVisitor : public Visitor {
private:
    long long last_value_ = 0; // Almacena el resultado temporal del último sub-árbol evaluado

public:
    long long evaluate(Expr* expr) {
        expr->accept(this); // Iniciamos el recorrido
        return last_value_;
    }

    // Al visitar un número literal, el valor de retorno es su propio valor
    void visit(IntLitExpr* node) override {
        last_value_ = node->value;
    }

    // Al visitar una expresión binaria, evaluamos recursivamente ambos lados
    void visit(BinaryExpr* node) override {
        node->left->accept(this);        // Evalúa el lado izquierdo
        long long left_val = last_value_; // Guarda el resultado izquierdo

        node->right->accept(this);        // Evalúa el lado derecho
        long long right_val = last_value_; // Guarda el resultado derecho

        // Aplica la operación correspondiente
        switch (node->op) {
            case BinaryOp::Add: last_value_ = left_val + right_val; break;
            case BinaryOp::Sub: last_value_ = left_val - right_val; break;
            case BinaryOp::Mul: last_value_ = left_val * right_val; break;
            case BinaryOp::Div: last_value_ = left_val / right_val; break;
            default: last_value_ = 0;
        }
    }

    // Implementaciones vacías para el resto de los métodos virtuales de Visitor
    void visit(Program* node) override {}
    void visit(FloatLitExpr* node) override {}
    void visit(BoolLitExpr* node) override {}
    void visit(CharLitExpr* node) override {}
    void visit(StringLitExpr* node) override {}
    void visit(IdExpr* node) override {}
    void visit(AssignExpr* node) override {}
    void visit(Block* node) override {}
    void visit(VarDeclStmt* node) override {}
    void visit(IfStmt* node) override {}
    // ... etc (todos los métodos virtuales puros de la clase base deben ser sobrescritos)
};
```

> **Consejo mental para diseñar y entender un Visitor:**
> La forma más fácil de razonar y construir un Visitor es pensar recursivamente en dos niveles:
> 1. **Los nodos hoja (caso base):** Comienza siempre implementando los nodos más sencillos que no tienen hijos (como `IntLitExpr` o `IdExpr`). Estos nodos hacen el trabajo directo (por ejemplo, devolver su propio valor o cargarlo en un registro) y detienen la recursión.
> 2. **Los nodos internos (paso inductivo):** Cuando programes un nodo con hijos (como `BinaryExpr` o `IfStmt`), no intentes resolver el recorrido de todo el subárbol en tu cabeza. Simplemente **asume** con fe que al llamar a `hijo->accept(this)`, el Visitor hará su trabajo correctamente y dejará el resultado donde se espera (por ejemplo, en un registro o en una variable temporal del Visitor). Bajo esa premisa, solo concéntrate en combinar los resultados de tus hijos.

---

## 5. Visitors Reales en el Compilador

Nuestro compilador implementa las siguientes clases principales derivadas de `Visitor`:

1. **`TypeChecker`**: 
   * Recorre el AST para verificar que las expresiones sean del tipo correcto y respete las reglas del alcance del compilador.
   * Guarda los tipos deducidos en sus variables miembro para validación.

2. **`CodeGenerator`**:
   * Genera las instrucciones ensamblador x86-64 correspondientes a cada nodo del AST.
   * Por ejemplo, al visitar un `BinaryExpr`, emite los `pushq`, `popq` y la instrucción aritmética respectiva (como `addq` o `subq`).

