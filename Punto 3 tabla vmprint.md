# Punto 3: Imprimir Tabla de Páginas (vmprint)

Este documento explica la implementación de la función de depuración `vmprint`, la llamada al sistema `dumpvm` y el programa de usuario `tdumpvm` para inspeccionar la tabla de páginas de un proceso en ejecución en xv6 (RISC-V).

## Funcionalidad

El sistema de memoria virtual de xv6 en RISC-V utiliza una **tabla de páginas jerárquica de 3 niveles**. El objetivo de este punto es poder visualizar cómo está mapeada la memoria virtual a la memoria física para un proceso en un momento dado, mostrando los niveles del árbol de paginación y los permisos de cada página.

### 1. Función de Depuración en el Kernel (`kernel/vm.c`)
Se implementaron dos funciones principales en el subsistema de memoria virtual:
- `vmprint(pagetable_t pagetable)`: Recibe la tabla de páginas raíz de un proceso e inicia el proceso de impresión mostrando la dirección de memoria base de la tabla.
- `_vmprint(pagetable_t pagetable, int level)`: Una rutina recursiva que explora los 512 índices (PTEs) de un nivel específico. 
  - Si un PTE es válido (flag `PTE_V`), imprime la entrada con una indentación basada en la profundidad (`.. ` por nivel).
  - Determina si la entrada apunta a un nivel inferior del árbol o si es una "hoja" (página terminal).
  - Para las páginas terminales, extrae e imprime sus banderas de acceso legibles: `R` (lectura), `W` (escritura), `X` (ejecución) y `U` (usuario).

### 2. Llamada al Sistema (`dumpvm`)
Para que un proceso desde el espacio de usuario pueda solicitar ver su propia tabla de páginas, se expuso esta funcionalidad a través de una syscall:
- Se le asignó el número **27** en `syscall.h`.
- Se registró el handler `sys_dumpvm` en `kernel/sysproc.c`.
- Internamente, `sys_dumpvm` simplemente accede a la estructura del proceso actual (`myproc()->pagetable`) y se la pasa a `vmprint()`, retornando 0 al finalizar. 

### 3. Programa Espacio de Usuario (`user/tdumpvm.c`)
Es un programa demostrativo que valida el crecimiento de la memoria del proceso:
1. Imprime su PID y ejecuta `dumpvm()` para mostrar la tabla de páginas **inicial** (donde se puede observar las páginas de código, datos, la pila y las páginas compartidas como el trampolín).
2. Solicita 4096 bytes extras de memoria (una página completa) usando la función dinámica de asignación `malloc` (que subyacentemente usa `sbrk`).
3. Modifica una posición de esa nueva memoria para asegurarse de que el kernel la asigne físicamente (Demand Paging / Lazy Allocation).
4. Vuelve a ejecutar `dumpvm()` para mostrar la tabla de páginas **actualizada**. Aquí se evidencia cómo se conectan nuevas ramas y hojas al árbol de paginación.

---

## Cómo Usarlo

Una vez compilado el sistema (con `make clean` y `make qemu`), el binario `_tdumpvm` estará disponible en el sistema.

1. Abre la terminal de xv6 e ingresa el comando:
   ```bash
   tdumpvm
   ```

### Salida Esperada e Interpretación

Verás una salida jerárquica con el siguiente formato general:
```text
=== tdumpvm [3]: Initial page table ===
page table 0x0000000087f6b000
.. 0: pte 0x0000000021fda801 pa 0x0000000087f6a000
.. .. 0: pte 0x0000000021fda401 pa 0x0000000087f69000
.. .. .. 0: pte 0x0000000021fdccdb pa 0x0000000087f73000 flags: R-XU
.. .. .. 1: pte 0x0000000021fdc85b pa 0x0000000087f72000 flags: R-XU
.. .. .. 2: pte 0x0000000021fdc0d7 pa 0x0000000087f70000 flags: RW-U
...
.. 255: pte 0x0000000021fdf401 pa 0x0000000087f7d000
.. .. 511: pte 0x0000000021fdf001 pa 0x0000000087f7c000
.. .. .. 510: pte 0x0000000021fdd007 pa 0x0000000087fa4000 flags: RW-U
.. .. .. 511: pte 0x0000000020001c0b pa 0x0000000080007000 flags: R-X-
```

**Comprendiendo el volcado:**
- **Niveles:** Los `..` representan la profundidad dentro de la estructura de 3 niveles de la tabla. Un nodo hoja tiene 3 impresiones de `..`.
- **Text (Código):** Las páginas con banderas `R-XU` indican que son ejecutables por un proceso de usuario.
- **Data (Variables/Heap/Stack):** Las páginas con banderas `RW-U` indican que el proceso puede leerlas y escribirlas, pero no ejecutarlas.
- **Trampoline / Trapframe:** Las áreas superiores en las direcciones más altas (como el índice 511 del último directorio) suelen carecer del flag `U` o son fijas para mapeos del sistema (ej. el trampolín suele ser `R-X-` para que el modo supervisor lo ejecute).
- Al comparar la primera tabla con la segunda en la ejecución, notarás que se agregan nuevas subramas/PTEs correspondientes a los 4096 bytes solicitados.