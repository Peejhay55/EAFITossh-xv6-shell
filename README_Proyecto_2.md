# EAFITos - Proyecto 2: Syscalls y Memoria Virtual

**Autor:** Pablo José Benítez Trujillo

**Clase:** Sistemas Operativos C2661-SI2004-4787

## Funcionalidades Implementadas
En este proyecto se amplió el kernel de xv6 para añadir nuevas llamadas al sistema (syscalls), herramientas de depuración de registros y funciones para el manejo e inspección del sistema de memoria virtual (paginación y page faults). A continuación se listan los hitos logrados:

### 1. Llamada al Sistema `hello`
Se implementó una llamada al sistema básica que cruza desde el espacio de usuario al kernel para imprimir un mensaje corto (`Hello from EAFITos kernel!`).
- **Programa de prueba:** `hello`
- **Técnicas usadas:** Transición S-mode/U-mode, intercepción de la instrucción `ecall` y log temporal en `usertrap()`.

### 2. Rastreo de Syscalls `trace` y `ttrace`
Se añadió un mecanismo persistente para rastrear (tracear) llamadas al sistema específicas de un proceso y sus hijos mediante una máscara de bits.
- **Programa de prueba:** `ttrace <mask> <comando>`
- **Técnicas usadas:** Almacenamiento de estado en `struct proc`, decodificación de argumentos mediante `argint()`, herencia de estado en `fork()`.

### 3. Visualización de la Tabla de Páginas `vmprint`
Extensión del subsistema de memoria virtual de xv6 para imprimir de forma jerárquica las traducciones de la tabla de páginas (PTEs) de un proceso con sus respectivas banderas de permisos (R, W, X, U).
- **Programa de prueba:** `tdumpvm`
- **Técnicas usadas:** Recursión sobre directorios de páginas, análisis de flags de RISC-V, demostración de crecimiento del heap (vía `malloc`/`sbrk`).

### 4. Permisos de Memoria y Page Faults `map_ro`
Evaluación de la robustez del hardware y el SO al proteger la memoria. Se instrumentó al kernel para asignar una página mapeada como "solo lectura" (Read-Only) al usuario.
- **Programa de prueba:** `tmemro`
- **Técnicas usadas:** Manejo del trap `scause 15` (Store Page Fault), uso inteligente de `mappages()` restringiendo la bandera `PTE_W`, recolección de basura (*garbage collection*) controlada para prevenir *kernel panics* tras abortar agresivamente un proceso culpable.

### 5. Inspección Cruda de Argumentos de Syscalls
Se instaló un inspector de los registros de hardware del trapframe (`a0`...`a5`) para comprobar que el paso de punteros por parte de `copyin`/`copyout` en xv6 detecta y rechaza el acceso a punteros inválidos sin hacer colapsar al sistema (ej. nulos o sin mapear).
- **Programa de prueba:** `tuargs`
- **Técnicas usadas:** Depuración HEX de punteros en los interceptores de `syscall.c`.

## Documentación Detallada
A lo largo del proyecto se generaron guías detalladas para cada funcionalidad. Consulte los archivos Markdown incluidos en el directorio raíz para profundizar:
- `Punto 1 hello.md`
- `Punto 2 Ttrace.md`
- `Punto 3 tabla vmprint.md`
- `Punto 4 tmemro.md`
- `Punto 5 tuargs.md`

## Pre-requisitos
Para compilar y ejecutar este proyecto sobre xv6, se requiere:
1. Entorno Linux/Unix (probado en Ubuntu).
2. RISC-V GNU toolchain (riscv64-unknown-elf-gcc), compatible con C11.
3. GNU Make.
4. QEMU configurado para RISC-V (qemu-system-riscv64).

## Guía paso a paso para correr los tests

### Compilación e Inicio
1. **Compilar y arrancar xv6:**
   Estando en la raíz del proyecto, ejecute:
   ```bash
   make clean
   make qemu
   ```
2. **Ejecución de Pruebas:**
   Una vez que xv6 haya cargado y vea el prompt original `$`, puede usar directamente los comandos creados:
   
   - `$ hello`
   - `$ ttrace 65536 echo Hola` *(65536 es la máscara para tracear write)*
   - `$ tdumpvm`
   - `$ tmemro`
   - `$ tuargs`

*(También puede usar esto dentro de la shell personalizada invocando `$ EAFITossh`)*