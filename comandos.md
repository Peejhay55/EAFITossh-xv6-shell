# Comandos de EAFITos

Acá te explicamos qué hace cada comando de la shell, sin tanto rollo.

---

## ¿Esto es de xv6?

Sí y no. EAFITos tiene **dos versiones**:

1. **Versión POSIX (Linux)**: Es la que está en la carpeta `src/`. Corre en cualquier Linux normal y usa las librerías estándar de C (glibc). Es la versión "completa".

2. **Versión xv6**: Está en el archivo `migration/EAFITossh.c`. Es la misma shell pero adaptada para correr dentro de **xv6**, que es un sistema operativo súper minimalista que se usa para enseñar cómo funcionan los sistemas operativos.

Los **comandos son los mismos** en ambas versiones (listar, leer, calc, buscar, etc.), pero la versión de xv6 tiene algunas limitaciones porque xv6 es muy básico:

| Cosa | Versión Linux | Versión xv6 |
|------|---------------|-------------|
| Colores en la terminal | Sí (ANSI) | No |
| Mostrar carpeta actual en el prompt | Sí (`EAFITos:~/carpeta$`) | No (solo `EAFITos$`) |
| Variables de entorno (`env`, `export`, `unset`) | Sí | No existen en xv6 |
| Calculadora (`calc`) | Con decimales | Solo números enteros |
| Fecha y hora (`tiempo`) | Fecha real (2026-02-27 10:30:00) | Solo "ticks" de uptime |
| Estadísticas de archivo | Permisos, fechas, todo | Sin permisos ni fechas (xv6 no los tiene) |
| Renombrar/mover archivos | Usa `rename()` | Usa `link()` + `unlink()` (porque xv6 no tiene `rename`) |

La estructura del parser (la parte que entiende lo que escribes) en la versión xv6 viene directamente del `sh.c` original de xv6 — ese que usa un árbol de structs (`execcmd`, `redircmd`, `pipecmd`, etc.) para representar los comandos. Pero todos los comandos en español (`listar`, `leer`, `crear`, etc.) son propios de EAFITos, no vienen de xv6.

En resumen: el "esqueleto" del parser/executor en xv6 es de xv6, pero los comandos y la funcionalidad son de EAFITos.

---

## Comandos en inglés (los clásicos)

### `cd`
Te mueve a otra carpeta. Si no le pones nada, te lleva a tu carpeta de inicio (HOME). Es como abrir una carpeta en el explorador de archivos, pero con texto.

Por debajo usa la llamada al sistema `chdir()`, que le dice al sistema operativo "oye, ahora mi directorio de trabajo es este otro". Todo lo que hagas después (listar archivos, crear cosas, etc.) va a ser relativo a esa carpeta.

En xv6 funciona igual (también tiene `chdir`), pero si no le das argumento te manda a `/` en vez de a HOME (porque xv6 no tiene variables de entorno).

```
cd Documents        ← entra a la carpeta Documents
cd ..               ← vuelve una carpeta atrás
cd                  ← te lleva al HOME (en Linux) o a / (en xv6)
cd /tmp             ← va directo a /tmp
```

### `exit`
Cierra la shell. Se acabó, chao. Si le pones un número, ese va a ser el "código de salida" — una forma de decirle al sistema "terminé bien" (0) o "algo salió mal" (cualquier otro número). Los programas usan esto para saber si el comando anterior funcionó o no.

```
exit           ← cierra con código 0 (todo bien)
exit 1         ← cierra con código 1 (indica que algo falló)
```

### `help`
Te muestra todos los comandos que existen con una descripción cortita en inglés. Es básicamente una tabla con el nombre del comando y qué hace. Si no sabes qué escribir, este es tu amigo.

```
help
```

### `env`
Te muestra todas las variables de entorno que tiene tu sistema. Las variables de entorno son como "datos globales" que los programas pueden leer. Por ejemplo, `HOME` es tu carpeta personal, `PATH` es donde el sistema busca los programas, `LANG` es tu idioma, etc.

**Nota:** Este comando NO existe en la versión xv6, porque xv6 no tiene variables de entorno.

```
env
```

Ejemplo de lo que te muestra:
```
HOME=/home/usuario
PATH=/usr/bin:/bin
LANG=es_CO.UTF-8
SHELL=/bin/bash
...un montón más
```

### `export`
Sirve para crear o cambiar una variable de entorno. Tienes que usar el formato `NOMBRE=VALOR`. Esto es útil cuando quieres que algún programa encuentre una configuración que tú le pones. La variable dura mientras la shell esté abierta.

**Nota:** No existe en la versión xv6.

```
export MI_VARIABLE=hola
export EDITOR=vim
```

### `unset`
Borra una variable de entorno. La elimina como si nunca hubiera existido. Si antes la creaste con `export`, con esto la quitas.

**Nota:** No existe en la versión xv6.

```
unset MI_VARIABLE
```

---

## Comandos en español

Estos son los que hacen a EAFITos especial — son comandos propios escritos en español para que sea más fácil usarlos sin saber inglés. Funcionan tanto en la versión Linux como en xv6 (con las diferencias que mencionamos arriba).

### `listar`
Te muestra lo que hay dentro de una carpeta (archivos y subcarpetas). Si no le dices cuál carpeta, te muestra la actual. Las carpetas aparecen con una `/` al final para que las diferencies de los archivos.

Automáticamente oculta los archivos que empiezan con `.` (los archivos ocultos de Linux), como `.bashrc` o `.git`.

Por debajo, en Linux usa `opendir()`/`readdir()` (funciones para leer directorios), y en xv6 lee el directorio directamente byte por byte con `read()` porque xv6 no tiene esas funciones.

```
listar                  ← muestra la carpeta actual
listar Documents        ← muestra lo que hay en Documents
```

Ejemplo de salida:
```
  archivo.txt
  notas.md
  proyecto/
  tarea.c
```

### `leer`
Te muestra el contenido de un archivo de texto en la pantalla. Básicamente lo abre, lo lee todito y lo imprime. Es como un `cat` de Linux pero en español.

Lee en bloques de 4KB (4096 bytes), así que no importa si el archivo es grande — lo maneja bien sin comerse toda la memoria.

```
leer mi_archivo.txt
leer /etc/hostname
```

### `tiempo`
Te dice la fecha y hora actual. Así de simple.

En Linux te da la fecha real con año, mes, día, hora, minutos y segundos. En xv6 no hay reloj de verdad, así que te muestra los "ticks" que lleva encendido el sistema (algo así como "han pasado X pulsos de reloj desde que prendiste la máquina").

```
tiempo
```

Salida en Linux:
```
2026-02-27 10:30:00
```

Salida en xv6:
```
Uptime: 4523 ticks
```

### `calc`
Una calculadora básica. Le pasas dos números y un operador en medio, y te da el resultado. Soporta suma (`+`), resta (`-`), multiplicación (`*`), división (`/`) y módulo (`%%` o `mod`, que es el residuo de una división).

En Linux trabaja con decimales (números con punto), así que puedes hacer cosas como `calc 10 / 3` y te da `3.33333`. En xv6 solo usa números enteros, así que `10 / 3 = 3` y ya.

Si intentas dividir por cero, te avisa y no se rompe.

```
calc 10 + 5       → 15
calc 20 / 4       → 5
calc 7 * 3        → 21
calc 10 - 3       → 7
calc 10 %% 3      → 1 (el residuo de 10 ÷ 3)
calc 10 mod 3     → 1 (lo mismo pero con la palabra "mod")
calc 7.5 + 2.3    → 9.8 (solo en Linux, en xv6 sería 7 + 2 = 9)
```

### `ayuda`
Igual que `help`, pero te muestra la lista de comandos con formato en español. Muestra todos los comandos disponibles (tanto los de inglés como los de español) con su descripción.

```
ayuda
```

### `salir`
Igual que `exit`, pero te dice "Hasta luego!" antes de cerrar. Más amigable. También acepta un código de salida opcional.

```
salir
salir 0
```

---

## Comandos de archivos

Estos comandos te permiten manejar archivos sin necesitar saber los comandos de Linux en inglés. Todos tienen protecciones para que no borres o sobreescribas cosas por accidente.

### `crear`
Crea un archivo vacío. Si el archivo ya existe, no lo toca y te avisa que ya está ahí — así no pierdes datos por accidente. Es útil para cuando quieres preparar un archivo antes de meterle contenido.

En Linux usa `fopen()`, en xv6 usa `open()` con la bandera `O_CREATE`.

```
crear notas.txt
  → Archivo 'notas.txt' creado.

crear notas.txt     (si ya existe)
  → crear: el archivo 'notas.txt' ya existe
```

### `eliminar`
Borra un archivo, pero primero te pregunta si estás seguro. Tienes que responder `s` o `S` para confirmar. Cualquier otra cosa (incluyendo solo darle enter) cancela la operación. Es una red de seguridad para no borrar algo importante.

En xv6 usa `unlink()` en vez de `remove()` (que no existe allá).

```
eliminar notas.txt
  → Eliminar 'notas.txt'? [s/N]: s
  → Archivo 'notas.txt' eliminado.

eliminar notas.txt
  → Eliminar 'notas.txt'? [s/N]: n
  → Operacion cancelada.
```

### `renombrar`
Le cambia el nombre a un archivo. Le das el nombre viejo y el nombre nuevo. No te deja si el nombre nuevo ya existe (para prevenir sobreescrituras).

En Linux usa `rename()`, pero en xv6 esa función no existe, así que lo hace en dos pasos: primero crea un "link" (un segundo nombre que apunta al mismo archivo) con `link()`, y después borra el nombre viejo con `unlink()`.

```
renombrar viejo.txt nuevo.txt
  → 'viejo.txt' renombrado a 'nuevo.txt'.
```

### `copiar`
Hace una copia de un archivo. Le das el archivo original y el nombre de la copia. Tampoco te deja si el destino ya existe. Lee el archivo original en pedazos de 512 bytes (en xv6) o 4KB (en Linux) y los escribe en el nuevo archivo. Si algo falla mientras copia, borra la copia incompleta para no dejarte archivos rotos.

```
copiar original.txt copia.txt
  → 'original.txt' copiado a 'copia.txt'.
```

### `mover`
Mueve un archivo de un lugar a otro. Si el destino ya existe, no lo hace. Internamente es similar a renombrar — en Linux usa `rename()`, en xv6 usa `link()` + `unlink()`. La diferencia con `renombrar` es más que nada conceptual: usas `mover` cuando quieres cambiar de carpeta, y `renombrar` cuando quieres cambiar el nombre.

```
mover archivo.txt carpeta/archivo.txt
  → 'archivo.txt' movido a 'carpeta/archivo.txt'.
```

### `buscar`
Busca un texto dentro de un archivo y te muestra las líneas donde lo encontró, con su número de línea. Es como un `grep` pero sencillito y en español. Va línea por línea buscando si tu texto aparece ahí (usando `strstr()`, que busca una cadena dentro de otra).

Si no encuentra nada, te lo dice. Si encuentra, te muestra cada línea que coincide y al final te dice cuántas encontró.

En xv6 la lectura línea por línea se tuvo que hacer a mano (leyendo carácter por carácter con `read()`), porque xv6 no tiene `fgets()`.

```
buscar "hola" mi_archivo.txt
  → 3: hola mundo
  → 7: dije hola otra vez
  →
  → 2 coincidencia(s) encontrada(s).

buscar "xyz" mi_archivo.txt
  → No se encontro 'xyz' en 'mi_archivo.txt'.
```

### `estadisticas`
Te da un montón de info sobre un archivo: su tamaño, tipo, permisos, cuándo se modificó, cuándo se accedió, y si es un archivo de texto también te dice cuántas líneas, palabras y caracteres tiene.

Usa la llamada al sistema `stat()` para obtener la información del archivo. Luego, para contar líneas/palabras/caracteres, abre el archivo y lo recorre carácter por carácter.

En xv6, la info es más limitada: no hay permisos (xv6 no los maneja) ni fechas de modificación, pero sí te muestra el número de inode y los links.

```
estadisticas mi_archivo.txt
```

Salida en Linux:
```
  Archivo:    mi_archivo.txt
  Tamano:     1234 bytes
  Tipo:       archivo regular
  Permisos:   rwxr-xr-x
  Modificado: 2026-02-27 10:30:00
  Accedido:   2026-02-27 10:35:00
  Lineas:     42
  Palabras:   300
  Caracteres: 1234
```

Salida en xv6:
```
  Archivo:    mi_archivo.txt
  Tamano:     1234 bytes
  Tipo:       archivo regular
  Links:      1
  Inode:      27
  Permisos:   N/A (xv6)
  Timestamps: N/A (xv6)
  Lineas:     42
  Palabras:   300
  Caracteres: 1234
```

---

## Funciones extra de la shell

Además de los comandos, la shell soporta estas cositas. Estas vienen del parser — en la versión Linux es un parser propio, y en la versión xv6 se usa el parser de `sh.c` de xv6 (el que usa structs como `execcmd`, `pipecmd`, `redircmd`, etc.).

- **Tuberías (`|`)**: Puedes encadenar comandos para que la salida de uno sea la entrada del otro. Internamente crea un "pipe" (un tubo) entre los dos procesos usando `pipe()` y `fork()`. Ejemplo: `ls | grep .c`
- **Redirección de salida (`>`)**: Manda la salida de un comando a un archivo en vez de a la pantalla. Si el archivo existe, lo sobreescribe. Ejemplo: `listar > archivos.txt`
- **Redirección de salida (append) (`>>`)**: Igual que `>`, pero en vez de sobreescribir, agrega al final del archivo. En xv6 esto se simula leyendo hasta el final del archivo y escribiendo ahí (porque xv6 no tiene `O_APPEND`). Ejemplo: `echo nueva linea >> log.txt`
- **Redirección de entrada (`<`)**: Hace que un comando lea desde un archivo en vez del teclado. Ejemplo: `wc -l < archivo.txt`
- **Ejecución en segundo plano (`&`)**: Pone un comando a correr de fondo para que puedas seguir usando la shell mientras se ejecuta. Te muestra el PID (el número de identificación del proceso). Ejemplo: `sleep 10 &`
- **Comandos externos**: Todo lo que no sea un comando interno se busca en el PATH del sistema (como `ls`, `grep`, `cat`, `gcc`, etc.). En la versión Linux usa `execvp()` que busca automáticamente en las carpetas del PATH. En xv6 usa `exec()` que necesita la ruta completa del programa.

---

## Estructura del archivo xv6 (`EAFITossh.c`)

El archivo `migration/EAFITossh.c` tiene todo metido en un solo archivo (en la versión Linux está separado en varios: `main.c`, `shell.c`, `parser.c`, `executor.c`, `builtins.c`, `utils.c`). Esto es porque en xv6 es más práctico tener un solo archivo por programa de usuario.

El archivo está organizado así:
1. **Funciones de utilidad** — cosas como `strstr()`, `strncmp()`, `strcat()` que xv6 no trae y hubo que escribir a mano.
2. **Banner y prompt** — lo que imprime cuando arranca la shell y antes de cada comando.
3. **Tabla de ayuda** — la lista de comandos con sus descripciones.
4. **Handlers de builtins** — las funciones que ejecutan cada comando (`builtin_cd`, `builtin_listar`, `builtin_calc`, etc.).
5. **Parser de xv6** — el que parsea pipes, redirecciones y listas de comandos (viene del `sh.c` original de xv6).
6. **Executor de xv6** — `runcmd()` y `fork1()`, que es la parte que realmente ejecuta los comandos con `fork()` + `exec()`.
7. **`main()`** — el loop principal que lee líneas, revisa si es un builtin, y si no, lo manda al parser+executor.
