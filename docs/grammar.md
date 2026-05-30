# Gramática del subconjunto de C++

Notación: `*` = cero o más, `+` = uno o más, `[x]` = opcional, `|` = alternativa.

---

## Programa

```
Program     ::= TopDecl*

TopDecl     ::= StructDecl
              | TemplateFuncDecl
              | FuncDecl
              | GlobalVarDecl
```

---

## Tipos

```
Type        ::= [const] BaseType PtrMod*
              | [const] id PtrMod*
              | [const] id < Type > PtrMod*    -- template instanciado
              | auto

BaseType    ::= int | long | float | double | bool | char | void | string

PtrMod      ::= * | &
```

---

## Declaraciones globales

```
GlobalVarDecl ::= [const] Type id [= Expr] ;

StructDecl    ::= struct id { MemberDecl* } ;
MemberDecl    ::= Type id ;

TemplateFuncDecl ::= template < typename id > FuncDecl

FuncDecl      ::= Type id ( ParamList ) Block

ParamList     ::= ε | Param (, Param)*
Param         ::= [const] Type [&] id [= Expr]
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

VarDeclStmt ::= [const] Type id [= Expr] ;
              | [const] Type id [ Expr ] ([ Expr ])* [= { InitList }] ;     -- array estático

ExprStmt    ::= Expr ;

IfStmt      ::= if ( Expr ) Stmt [else Stmt]

WhileStmt   ::= while ( Expr ) Stmt

ForStmt     ::= for ( ForInit ; Expr ; Expr ) Stmt         -- for clásico
              | for ( [const] Type id : Expr ) Stmt        -- range-based for

ForInit     ::= [const] Type id [= Expr]
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

Assign  ::= LogicOr [AssignOp Assign]
AssignOp ::= = | += | -= | *= | /= | %= | &= | |=

LogicOr  ::= LogicAnd (|| LogicAnd)*
LogicAnd ::= Equality (&& Equality)*
Equality ::= Relat ((== | !=) Relat)*
Relat    ::= Add ((< | > | <= | >=) Add)*
Add      ::= Mul ((+ | -) Mul)*
Mul      ::= Unary ((* | / | %) Unary)*

Unary   ::= (- | ! | ~ | * | & | ++ | --) Unary
          | static_cast < Type > ( Expr )
          | new Type [ Expr ]               -- array dinámico
          | new Type ( ArgList )            -- objeto dinámico
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
          | id { InitList }     -- struct literal / aggregate init
          | Lambda

InitList ::= ε | Expr (, Expr)*
ArgList  ::= ε | Expr (, Expr)*
```

---

## Lambdas

```
Lambda       ::= [ CaptureList ] ( ParamList ) [-> Type] Block

CaptureList  ::= ε
               | &                        -- captura todo por referencia
               | =                        -- captura todo por valor
               | CaptureItem (, CaptureItem)*

CaptureItem  ::= [&] id
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

- La gramática de `IfStmt` es ambigua (dangling else). Se resuelve asociando el `else` al `if` más cercano.
- `id { InitList }` como Primary es ambiguo en condiciones de `if`/`while`/`for` (¿struct literal o bloque?). Se resuelve con un flag contextual igual que en el compilador de referencia.
- Las conversiones implícitas (`int` → `float`, etc.) se manejan en el semántico, no en la gramática.
- Templates: solo se soportan funciones template con un parámetro de tipo (`typename T`). No se soportan clases template.
