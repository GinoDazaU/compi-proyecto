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

BaseType    ::= int | float | bool | char | void | string

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

IfStmt      ::= if ( Expr ) Block [else (Block | IfStmt)]

WhileStmt   ::= while ( Expr ) Block

ForStmt     ::= for ( ForInit ; Expr ; Expr ) Block         -- for clásico
              | for ( [const] Type id : Expr ) Block        -- range-based for

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

- Las conversiones implícitas (`int` → `float`, etc.) se manejan en el semántico, no en la gramática.
- Templates: solo se soportan funciones template con un parámetro de tipo (`typename T`). No se soportan clases template.

---

## Funciones Incorporadas (Built-ins)

El compilador provee las siguientes funciones predefinidas de forma global para la salida estándar. No requieren una declaración previa para ser utilizadas:

- **`print(arg1, arg2, ...)`**: Acepta uno o más argumentos de tipo básico (`int`, `float`, `bool`, `char`, `string`) y los imprime en la consola de manera secuencial sin agregar un salto de línea al final.
- **`println(arg1, arg2, ...)`**: Funciona igual que `print`, pero añade automáticamente un salto de línea (`\n`) al final de la impresión.
