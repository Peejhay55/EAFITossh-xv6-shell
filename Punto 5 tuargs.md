# Punto 5: Argumentos de syscalls y robustez (Punteros de usuario)

Este documento detalla la implementación de depuración para la validación de registros crudos en la entrada de las llamadas al sistema y el programa `tuargs` para probar la robustez del kernel ante punteros inválidos.

## Funcionalidad

El objetivo consiste en entender y evidenciar cómo los argumentos (enteros y punteros) cruzan la frontera entre el espacio de usuario (U-mode) y el de supervisor (S-mode) alojándose en los registros `a0..a5`, y verificar que el control de punteros (usando `copyin`/`copyout` o `argaddr`) de xv6 sea completamente robusto en devolver un código de error (`-1`) sin "entrar en pánico" (Kernel Panic).

### 1. Depuración de Registros en `kernel/syscall.c`
Se amplió el bloque de inspección (que se usa durante la syscall `trace`):
- Antes de que la correspondiente función manejadora (handler) ejecute su lógica interna, se capturan los valores crudos directos de los primeros 6 registros del framework de trampas: `trapframe->a0` hasta `trapframe->a5`.
- A continuación, se decodifican asumiéndolos primero como enteros con `argint(0)`, `argint(1)` y `argint(2)`, y también como puntero con `argaddr(0)`.
- Se imprimen tanto los registros hexadecimales `RAW` como los `DECODED` generados por el helper. De esta forma, se evidencia que los parámetros efectivamente llegaron al kernel a través de registros de hardware.

### 2. Programa de Usuario de Pruebas `user/tuargs.c`
Es un ejecutable que automatiza un número de interacciones con el kernel pasando argumentos mixtos.
Se vale de la syscall `trace` para activar la supervisión antes descrita en las funciones `read`, `write`, `open` y `hello`.

- **Test A - Punteros válidos:** Usa `open`, `read` y `write` con buffers o punteros a cadenas completamente válidas (Ej. `"README"`) demostrando el flujo decodificado natural y el éxito de la llamada.
- **Test B - Punteros `NULL` (0x0):** Inyecta el temido puntero nulo a dichas funciones (`(void*)0x0`). En S-mode, `copyin`/`copyout` evitan el acceso directo a la página cero protegiendo al SO y obligando a la llamada a abortar retornando -1.
- **Test C - Punteros de espacio alto / Kernel (0xffffffff...):** Le ordena a sus respectivas llamadas al sistema a buscar datos fuera del espacio autorizado del usuario. Aquí el kernel vuelve a interceder, evitando una catástrofe de acceso denegado y retornando -1 de forma estricta.

---

## Cómo Usarlo

Una vez compilado el sistema operativo (con `make clean` y `make qemu`), entra al shell de la máquina virtual e ingresa el comando:

```bash
tuargs
```

### Salida Esperada

Verás información parecida a la siguiente (por cada syscall forzada a fallar):
```text
3 tuargs: syscall open (num=15)
  RAW (a0-a5): 0x0 0x0 0x8 0x69 0x41 0x0
  DECODED args: argint(0,1,2)=0, 0, 8 | argaddr(0)=0x0
```

Se nota cómo `argint` trata a un `0x0` o a un `0xffffffff...` como números comunes, pero los helpers en `sys_open` o dentro de `copyin`/`copyout` al revisar los límites de tamaño en `p->sz` identifican la petición peligrosa. 

Finalmente, el programa listará:
```text
Terminado con exito. Resultados invalidos: open(0x0)=-1, open(0xfff...)=-1
```
Demostrando que **ninguna de las llamadas letales causó un Kernel Panic ni cerró bruscamente el entorno**, ya que el OS detectó los fallos proactivamente, retornando limpiamente el estado correspondiente.