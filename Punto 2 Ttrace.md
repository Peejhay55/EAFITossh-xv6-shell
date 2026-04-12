# Punto 2: Syscall Trace (ttrace)

Este documento detalla la funcionalidad y el uso de la llamada al sistema `trace` y el comando de usuario `ttrace` implementados en xv6.

## Funcionalidad

La funcionalidad principal radica en permitir la inspección en tiempo de ejecución (rastreo o *tracing*) de las llamadas al sistema (syscalls) invocadas por un proceso específico y sus procesos hijos. 

### 1. La Syscall `trace(int mask)`
Se añadió la llamada al sistema `sys_trace` al kernel, la cual recibe como parámetro una **máscara de bits** (`mask`). 
- El kernel guarda esta máscara dentro de la estructura del proceso actual (`myproc()->trace_mask`).
- Cuando el proceso invoca `fork()`, esta máscara se hereda a los procesos hijos, garantizando que el rastreo sea persistente a lo largo de la ejecución.

### 2. Intercepción en el Kernel (`kernel/syscall.c`)
Antes de ejecutar la función manejadora de cualquier llamada al sistema, el kernel intercepta la ejecución y verifica si el bit correspondiente a dicha syscall está activo en la máscara del proceso:
`p->trace_mask & (1 << num)`
- Si el bit está activo, el kernel decodifica los argumentos de la llamada y manda a imprimir a la consola: el PID, el nombre del programa, el nombre de la syscall, su número interno y el valor de los primeros 3 argumentos decodificados usando `argint()`.

### 3. Comando de Usuario (`user/ttrace.c`)
Es un programa (*wrapper*) que sirve como punto de entrada. Recibe por consola la máscara deseada y el comando a ejecutar:
1. Extrae el número de la máscara.
2. Invoca la syscall `trace(mask)` para habilitar el rastreo en sí mismo.
3. Invoca `exec()` sobre el comando solicitado. Al usar `exec`, la imagen de memoria del programa `ttrace` se reemplaza por la del nuevo comando, pero **el proceso en el kernel es el mismo**, reteniendo la máscara de trace y rastreando silenciosamente toda la ejecución del nuevo programa.

---

## Cómo Usarlo

El comando `ttrace` se invoca de la siguiente manera desde la consola de xv6:

```bash
ttrace <mask> <comando> [argumentos...]
```

### Cálculo de la Máscara (Mask)
La máscara funciona por potencias de 2. Para rastrear una syscall cuyo número de identificación es `N`, se debe sumar el valor de $2^N$ a la máscara. Los números de las llamadas se encuentran en `kernel/syscall.h`.

**Ejemplos de algunas llamadas clave:**
- `read`   (num = 5)  → $2^5  = 32$
- `open`   (num = 15) → $2^{15} = 32768$
- `write`  (num = 16) → $2^{16} = 65536$

### Ejemplos Prácticos

1. **Rastrear solo la escritura a consola (`write`):**
   ```bash
   ttrace 65536 echo Hola_Mundo
   ```
   **Resultado:** El kernel imprimirá la información de cada vez que `echo` use `write` para imprimir un carácter o una cadena por la salida estándar.

2. **Rastrear solo la lectura de archivos (`read`):**
   ```bash
   ttrace 32 cat README
   ```
   **Resultado:** Verás las llamadas interceptadas informando cómo el comando `cat` pide porciones de bytes al kernel desde el archivo abierto.

3. **Rastrear operaciones de apertura, lectura y escritura simultáneamente:**
   Sumamos todos sus valores: $32768$ (`open`) + $32$ (`read`) + $65536$ (`write`) = **$98336$**.
   ```bash
   ttrace 98336 grep xv6 README
   ```
   **Resultado:** Podrás observar todo el flujo vital de entrada/salida de `grep`. Registrará la apertura del archivo (syscall `open`), los bloques de lectura durante la búsqueda (syscall `read`) y las impresiones finales correspondientes en pantalla (syscall `write`).