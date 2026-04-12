# Punto 1: Syscall Hello

Este documento detalla la implementación de la llamada al sistema `hello()` y su correspondiente programa de usuario, demostrando el flujo completo (end-to-end) desde el espacio de usuario (user space) hasta el espacio del núcleo (kernel space) en xv6.

## Funcionalidad

El objetivo de este punto es crear una llamada al sistema simple, entender cómo se registran las interrupciones por software y añadir instrumentación (logs) en el proceso de captura de la excepción.

La implementación consta de varias capas conectadas entre sí:

### 1. Espacio de Usuario (`user/hello.c`)
Se creó un programa ejecutable independiente llamado `hello`. Este programa:
- Incluye las librerías base de xv6 (`user.h`, etc.).
- Invoca la función `hello()` expuesta por el sistema operativo.
- Al ejecutarse, hace la transición al kernel y posteriormente termina su ejecución con un `exit(0)`.

### 2. Interfaz de Syscall (`usys.pl` y `syscall.h`)
Para que el programa en C pueda comunicarse con el kernel:
- Se añadió el prototipo `int hello(void);` en `user/user.h`.
- Se registró el entry `hello` en el script `user/usys.pl`, el cual se encarga de generar el código en ensamblador (`usys.S`) que emite la instrucción `ecall` de RISC-V.
- Se le asignó el identificador único **25** (`#define SYS_hello 25`) en `kernel/syscall.h`. Cuando se hace el `ecall`, este número se carga en el registro `a7`.

### 3. Trampa y Dispatcher (`kernel/trap.c` y `kernel/syscall.c`)
Cuando la CPU ejecuta el `ecall`, el control pasa a la función `usertrap()` en el kernel:
- El kernel detecta que la causa del trap fue una llamada al sistema porque el registro `scause` es igual a **8** (Environment call from U-mode).
- **Logs temporales:** Se añadió instrumentación específica dentro de `usertrap()`. Si la llamada al sistema corresponde a `hello` (revisando si `p->trapframe->a7 == 25`), imprime un log indicando el evento: `usertrap(): syscall scause=8 num=25`.
- Luego de registrar el log, el control se delega a `syscall()`, que usa el número de llamado para enrutar la ejecución hacia `sys_hello`.

### 4. Ejecución en el Kernel (`kernel/sysproc.c`)
Finalmente, se ejecuta la función `sys_hello()`:
- Imprime directamente desde el kernel el mensaje: `"Hello from EAFITos kernel!\n"`.
- Retorna un entero fijo (`0`), el cual es depositado en el registro de retorno (`a0`) del trapframe, completando exitosamente la llamada al sistema.

---

## Cómo Usarlo

Una vez compilado el sistema operativo (mediante `make clean` y `make qemu`), el programa binario `_hello` es insertado automáticamente en el sistema de archivos (`fs.img`).

1. Inicia xv6 ejecutando:
   ```bash
   make qemu
   ```
2. Dentro de la terminal de xv6 (`$`), simplemente escribe el nombre del comando:
   ```bash
   hello
   ```

### Salida Esperada
Al ejecutar el comando, se mostrará en pantalla primero el log interceptado por la trampa del hardware, y posteriormente el mensaje impreso por la lógica del sistema:

```text
$ hello
usertrap(): syscall scause=8 num=25
Hello from EAFITos kernel!
```