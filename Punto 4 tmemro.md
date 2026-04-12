# Punto 4: Permisos de memoria en acción (Page Fault)

Este documento explica la implementación de la syscall `map_ro`, el manejo extendido en subrutinas de traps y el programa de usuario `tmemro` para visualizar la respuesta del kernel ante violaciones de acceso de memoria.

## Funcionalidad

EL objetivo de este componente es comprobar que el hardware y el sistema operativo colaboran para proteger las áreas de memoria, validando las banderas de acceso (como `PTE_R` sin `PTE_W`) y reaccionando adecuadamente en caso de una violación (*page fault*).

### 1. Llamada al Sistema (`map_ro`)
Se ha introducido la syscall `sys_map_ro(void *va)` la cual mapea una página de memoria de sólo lectura:
- Recibe una dirección virtual (`va`), forzándola a comenzar en el límite de la página mediante redondeo hacia abajo (`PGROUNDDOWN`).
- Asigna físicamente una nueva página del sistema usando `kalloc()`.
- Inicializa la página con un mensaje hardcodeado desde el kernel invocando rutinas del nivel de supervisor.
- Posteriormente, mapea la traducción de la página usando la función base del sistema `mappages(...)`, pasándole las banderas `PTE_R` (Lectura) y `PTE_U` (Modo Usuario), **omitiendo la bandera de escritura (`PTE_W`)**.

### 2. Detección en las Instrucciones de Retorno (`trap.c`)
En RISC-V, cuando se viola una política de acceso (como intentar modificar un área protegida contra escritura), el hardware activa el bit correspondiente y salta a la función general del supervisor (`usertrap()`).
Se introdujo una modificación para interceptar el código de causa especifico:
- El registro `scause` número **15** define *"Store Page Fault"* (fallo por almacenamiento).
- Si dicho evento ocurre de forma controlada en un usuario e impacta en las protecciones, el kernel emite un log imprimiendo el número de fallo del hardware y la dirección de memoria exacta que lo provocó (`r_stval()`).
- Inmediatamente, la tarea es clasificada como inválida o peligrosa y matada de forma limpia invocando `setkilled(p)`, logrando que en el siguiente tick del temporizador el sistema ceda su proceso y la shell no colapse, regresando el control iterativo al usuario.

### 3. Programa de Usuario (`tmemro.c`)
Se ha configurado un programa ejecutable que evalúa exitosamente todo el caso de control:
1. Emplea la nueva función para enrutar una vista de una dirección en específico `0x40000000` simulada, instanciando allí una hoja de memoria de la jerarquía paginada que almacena la string redactada del kernel.
2. El programa accede de forma transparente para leer el vector del dato por consola e imprimirlo en tiempo de ejecución.
3. Se trata de escribir localmente sobrescribiendo por la fuerza el segmento `[0]` de la referencia de texto de `va`. El proceso aborta en milisegundos tras chocar con la bandera del TLB del *hardware* que dispara el page fault.

---

## Cómo Usarlo

Una vez compilado el sistema con QEMU, en la shell del OS digita:

```bash
tmemro
```

### Salida Esperada

Notarás un flujo iterativo como el siguiente:

```text
$ tmemro
tmemro: mapeando pagina solo-lectura en 0x0000000040000000
tmemro: leyendo pagina leida... [Mensaje corto de prueba desde el kernel (solo lectura)]
tmemro: intentando escribir en pagina RO, deberia fallar...
usertrap(): store page fault scause=15 stval=0x40000000
$ 
```

**Interpretación de los eventos:**
- Observarás que por medio de `printf`, los datos originados desde el kernel cruzan la barrera a `user-space` exitosamente, ya que los accesos por lectura al bloque `0x40000000` son totalmente permitidos.
- Las intenciones escritas fuerzan al Hardware del RISC-V a ceder e informar la detención forzosa por parte del OS la cual interceptamos en log de `store page fault`, el cual incluye tanto el factor causante (15) como el puntero del crimen `stval`, para luego retornar y delegar nuevamente a la consola de inicio en un nuevo estado de aislamiento intacto.