# Cortocircuito en `&&` y `||`

## Problema

La primera versión generaba `&&` y `||` con un AND/OR bit a bit del byte bajo
(`and %cl, %al` / `or %cl, %al`), evaluando siempre ambos lados. Esto tenía dos
fallas:

- Operandos que no son `0/1` daban resultados incorrectos: `2 && 1` → `0`,
  `2 || 0` → `2`, `6 && 3` → `2`.
- Sin cortocircuito, el lado derecho se evaluaba siempre. Con punteros, el idioma
  `p != nullptr && p->x` provoca segfault al desreferenciar un puntero nulo.

## Solución

`&&` y `||` se generan con cortocircuito y saltos. Se evalúa el lado izquierdo,
se normaliza a `0/1` (`cmpq $0` + `setne`, o `ucomisd` contra 0 para `float`) y,
si ya determina el resultado, se salta sin evaluar el derecho. El resultado
siempre es `0` o `1`.

Cubierto por `tests/e2e/binary_logic_short.txt`.
