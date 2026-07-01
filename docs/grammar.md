# Gramática del subconjunto de C++

Notación: `*` = cero o más, `+` = uno o más, `[x]` = opcional, `|` = alternativa.

---

## Programa

```
Program     ::= TopDecl*

TopDecl     ::= StructDecl
              | FuncDecl
```

---

## Tipos

```
Type        ::= BaseType PtrMod*
              | id PtrMod*
              | auto

BaseType    ::= int | float | bool | char | void | string

PtrMod      ::= *
```

---

## Declaraciones globales

```
StructDecl    ::= struct id { MemberDecl* } ;
MemberDecl    ::= Type id ;

FuncDecl      ::= Type id ( ParamList ) Block

ParamList     ::= ε | Param (, Param)*
Param         ::= Type id
```

---

## Bloque y sentencias

```
Block       ::= { Stmt* }

Stmt        ::= Block
              | VarDeclStmt
              | ExprStmt
              | IfStmt
              | WhileStmt
              | ForStmt
              | ReturnStmt
              | BreakStmt
              | ContinueStmt
              | DeleteStmt

VarDeclStmt ::= Type id [= Expr] ;
              | Type id [ Expr ] ([ Expr ])* [= { InitList }] ;     -- array estático

ExprStmt    ::= Expr ;

IfStmt      ::= if ( Expr ) Block [else (Block | IfStmt)]

WhileStmt   ::= while ( Expr ) Block

ForStmt     ::= for ( ForInit ; Expr ; Expr ) Block         -- for clásico

ForInit     ::= Type id [= Expr]
              | Expr
              | ε

ReturnStmt  ::= return [Expr] ;
BreakStmt   ::= break ;
ContinueStmt ::= continue ;
DeleteStmt  ::= delete [[ ]] Expr ;
```

---

## Expresiones (precedencia de menor a mayor)

```
Expr    ::= Assign

Assign  ::= LogicOr [= Assign]

LogicOr  ::= LogicAnd (|| LogicAnd)*
LogicAnd ::= Equality (&& Equality)*
Equality ::= Relat ((== | !=) Relat)*
Relat    ::= Add ((< | > | <= | >=) Add)*
Add      ::= Mul ((+ | -) Mul)*
Mul      ::= Unary ((* | / | %) Unary)*

Unary   ::= (- | ! | * | & | ++ | --) Unary
          | new Type [ Expr ]               -- array dinámico
          | new Type                        -- objeto dinámico (campos en cero)
          | Postfix

Postfix ::= Primary (PostfixOp)*
PostfixOp ::= [ Expr ]          -- indexado
            | ( ArgList )       -- llamada a función
            | . id              -- acceso a miembro
            | -> id             -- acceso por puntero
            | ++
            | --

Primary ::= id
          | IntLit
          | FloatLit
          | BoolLit
          | CharLit
          | StringLit
          | ( Expr )

InitList ::= ε | Expr (, Expr)*
ArgList  ::= ε | Expr (, Expr)*
```

---

## Tokens literales

```
IntLit    ::= [0-9]+
FloatLit  ::= [0-9]+ . [0-9]+
BoolLit   ::= true | false
CharLit   ::= ' cualquier_caracter '
StringLit ::= " cualquier_secuencia "
id        ::= [a-zA-Z_][a-zA-Z0-9_]*
```

---

## Notas

- Las conversiones implícitas (`int` → `float`, etc.) se manejan en el semántico, no en la gramática.

---

## Funciones Incorporadas (Built-ins)

El compilador provee las siguientes funciones predefinidas de forma global para la salida estándar. No requieren una declaración previa para ser utilizadas:

- **`print(arg1, arg2, ...)`**: Acepta uno o más argumentos de tipo básico (`int`, `float`, `bool`, `char`, `string`) y los imprime en la consola de manera secuencial sin agregar un salto de línea al final.
- **`println(arg1, arg2, ...)`**: Funciona igual que `print`, pero añade automáticamente un salto de línea (`\n`) al final de la impresión.
