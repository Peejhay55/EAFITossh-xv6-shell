# Comandos de EAFITos

Acá te explicamos qué hace cada comando de la shell (`EAFITossh.c`), cómo está hecho por dentro y cómo usarlo.

---

## Comandos básicos

### `cd`
Te mueve a otra carpeta. Si no le pones nada, te lleva a `/`. Es como abrir una carpeta en el explorador de archivos, pero con texto.

Por debajo usa la llamada al sistema `chdir()`, que le dice al sistema operativo "oye, ahora mi directorio de trabajo es este otro". Todo lo que hagas después (listar archivos, crear cosas, etc.) va a ser relativo a esa carpeta.

```c
// De EAFITossh.c
int
builtin_cd(char **argv, int argc)
{
  char *path = (argc > 1) ? argv[1] : "/";
  if (chdir(path) < 0) {
    fprintf(2, "cannot cd %s\n", path);
    return 1;
  }
  return 0;
}
```

Cómo se usa:
```
cd Documents        ← entra a la carpeta Documents
cd ..               ← vuelve una carpeta atrás
cd                  ← va a /
cd /user            ← va directo a /user
```

### `exit`
Cierra la shell. Se acabó, chao. Si le pones un número, ese va a ser el "código de salida" — una forma de decirle al sistema "terminé bien" (0) o "algo salió mal" (cualquier otro número).

```c
// De EAFITossh.c
int
builtin_exit_cmd(char **argv, int argc)
{
  if (argc > 1)
    g_ctx.last_exit_status = atoi(argv[1]);
  g_ctx.running = 0;
  return g_ctx.last_exit_status;
}
```

Cómo se usa:
```
exit           ← cierra con código 0 (todo bien)
exit 1         ← cierra con código 1 (indica que algo falló)
```

### `help`
Te muestra todos los comandos que existen con una descripción cortita. Es básicamente una tabla con el nombre del comando y qué hace. Recorre una tabla (`help_table`) definida en el código e imprime cada entrada.

```c
// De EAFITossh.c
int
builtin_help(void)
{
  fprintf(1, "%s -- Built-in commands:\n\n", EAFITOS_NAME);
  for (struct help_entry *h = help_table; h->name != 0; h++) {
    fprintf(1, "  ");
    print_padded(1, h->name, 14);
    fprintf(1, " %s\n", h->desc);
  }
  fprintf(1, "\nAll other commands are searched in the filesystem.\n");
  return 0;
}
```

Cómo se usa:
```
help
```

---

## Comandos en español

### `listar`
Te muestra lo que hay dentro de una carpeta (archivos y subcarpetas). Si no le dices cuál carpeta, te muestra la actual. Las carpetas aparecen con una `/` al final para que las diferencies de los archivos.

Automáticamente oculta las entradas que empiezan con `.`. Para leer el contenido del directorio usa `read()` directamente sobre el descriptor de archivo del directorio, leyendo estructuras `dirent` una por una. Luego usa `stat()` en cada entrada para saber si es carpeta o archivo.

```c
// De EAFITossh.c
struct dirent de;
while (read(fd, &de, sizeof(de)) == sizeof(de)) {
  if (de.inum == 0)
    continue;
  // Ignora los archivos que empiezan con punto
  if (de.name[0] == '.')
    continue;
  // ...
  if (stat(fullpath, &entry_st) >= 0 && entry_st.type == T_DIR)
    fprintf(1, "  %s/\n", name);
  else
    fprintf(1, "  %s\n", name);
}
```

Cómo se usa:
```
listar                  ← muestra la carpeta actual
listar user             ← muestra lo que hay en la carpeta user
```

Ejemplo de salida:
```
  archivo.txt
  notas.md
  proyecto/
  tarea.c
```

### `leer`
Te muestra el contenido de un archivo de texto en la pantalla. Lo abre, lo lee en bloques de 512 bytes (`BUF_SIZE`) y los imprime. Es como un `cat` de Linux pero en español.

```c
// De EAFITossh.c
int
builtin_leer(char **argv, int argc)
{
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(2, "leer: cannot open %s\n", argv[1]);
    return 1;
  }

  char buf[BUF_SIZE];
  int n;
  while ((n = read(fd, buf, sizeof(buf))) > 0)
    write(1, buf, n);

  close(fd);
  return 0;
}
```

Cómo se usa:
```
leer mi_archivo.txt
leer README
```

### `tiempo`
Te dice la fecha y hora actual (ajustada a Colombia, UTC-5). El sistema usa `rtctime()` para obtener los segundos transcurridos desde el 1 de enero de 1970, y luego hace la conversión manual a año, mes, día, hora, minutos y segundos con aritmética entera.

```c
// De EAFITossh.c
int utc_offset = -5;
int epoch = rtctime();
epoch += utc_offset * 3600;

int secs_in_day = epoch % 86400;
int hours   = secs_in_day / 3600;
int minutes = (secs_in_day % 3600) / 60;
int seconds = secs_in_day % 60;
// ...convierte 'days' a año/mes/día con un algoritmo de calendario...
fprintf(1, "Fecha: %s %d, %d\n", meses[m], d, y);
fprintf(1, "Hora:  %d:%d%d:%d%d %s (UTC%d)\n", h12, ...);
```

Cómo se usa:
```
tiempo
```

Salida:
```
Fecha: Feb 27, 2026
Hora:  10:30:00 AM (UTC-5)
```

### `calc`
Una calculadora básica de enteros. Le pasas dos números y un operador en medio, y te da el resultado. Soporta suma (`+`), resta (`-`), multiplicación (`*`), división (`/`) y módulo (`%` o `mod`, que es el residuo de una división). Si intentas dividir por cero, te avisa y no explota.

```c
// De EAFITossh.c
int a = atoi(argv[1]);
int b = atoi(argv[3]);
char *op = argv[2];
int result;

if (strcmp(op, "+") == 0)       result = a + b;
else if (strcmp(op, "-") == 0)  result = a - b;
else if (strcmp(op, "*") == 0)  result = a * b;
else if (strcmp(op, "/") == 0) {
  if (b == 0) { fprintf(2, "calc: division por cero\n"); return 1; }
  result = a / b;
} else if (strcmp(op, "%") == 0 || strcmp(op, "mod") == 0) {
  if (b == 0) { fprintf(2, "calc: division por cero\n"); return 1; }
  result = a % b;
}
fprintf(1, "%d\n", result);
```

Cómo se usa:
```
calc 10 + 5       → 15
calc 20 / 4       → 5
calc 7 * 3        → 21
calc 10 - 3       → 7
calc 10 % 3       → 1 (el residuo de 10 ÷ 3)
calc 10 mod 3     → 1 (lo mismo pero con la palabra "mod")
```

### `ayuda`
Igual que `help`, pero muestra la lista con un encabezado en español. Recorre la misma tabla `help_table` e imprime cada comando con su descripción.

```c
// De EAFITossh.c
int
builtin_ayuda(void)
{
  fprintf(1, "\n  %s -- Comandos disponibles:\n\n", EAFITOS_NAME);
  for (struct help_entry *h = help_table; h->name != 0; h++) {
    fprintf(1, "    ");
    print_padded(1, h->name, 14);
    fprintf(1, " %s\n", h->desc);
  }
  fprintf(1, "\n  Otros comandos se buscan en el sistema de archivos.\n\n");
  return 0;
}
```

Cómo se usa:
```
ayuda
```

### `salir`
Igual que `exit`, pero te dice "Hasta luego!" antes de cerrar. También acepta un código de salida opcional.

```c
// De EAFITossh.c
int
builtin_salir(char **argv, int argc)
{
  fprintf(1, "Hasta luego!\n");
  return builtin_exit_cmd(argv, argc);
}
```

Cómo se usa:
```
salir
salir 0
```

---

## Comandos de archivos

Estos comandos te permiten manejar archivos sin necesitar saber los comandos en inglés. Todos tienen protecciones para que no borres o sobreescribas cosas por accidente.

### `crear`
Crea un archivo vacío. Si el archivo ya existe, no lo toca y te avisa — así no pierdes datos por accidente. Usa `stat()` para verificar si ya existe antes de intentar crearlo con `open()` usando la bandera `O_CREATE`.

```c
// De EAFITossh.c
struct stat st;
if (stat(argv[1], &st) >= 0) {
  fprintf(2, "crear: el archivo '%s' ya existe\n", argv[1]);
  return 1;
}

int fd = open(argv[1], O_CREATE | O_WRONLY);
if (fd < 0) {
  fprintf(2, "crear: cannot create %s\n", argv[1]);
  return 1;
}
close(fd);
fprintf(1, "Archivo '%s' creado.\n", argv[1]);
```

Cómo se usa:
```
crear notas.txt
  → Archivo 'notas.txt' creado.

crear notas.txt     (si ya existe)
  → crear: el archivo 'notas.txt' ya existe
```

### `eliminar`
Borra un archivo, pero primero te pregunta si estás seguro. Tienes que responder `s` o `S` para confirmar. Cualquier otra cosa (incluyendo solo darle enter) cancela la operación. Usa `unlink()` para borrar el archivo del sistema de archivos.

```c
// De EAFITossh.c
fprintf(1, "Eliminar '%s'? [s/N]: ", argv[1]);

char respuesta[16];
gets(respuesta, sizeof(respuesta));

if (respuesta[0] == 's' || respuesta[0] == 'S') {
  if (unlink(argv[1]) < 0) {
    fprintf(2, "eliminar: cannot delete %s\n", argv[1]);
    return 1;
  }
  fprintf(1, "Archivo '%s' eliminado.\n", argv[1]);
} else {
  fprintf(1, "Operacion cancelada.\n");
}
```

Cómo se usa:
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

Se implementa en dos pasos: primero crea un enlace (un segundo nombre que apunta al mismo archivo) con `link()`, y después borra el nombre viejo con `unlink()`.

```c
// De EAFITossh.c
if (link(argv[1], argv[2]) < 0) {
  fprintf(2, "renombrar: cannot link %s to %s\n", argv[1], argv[2]);
  return 1;
}
if (unlink(argv[1]) < 0) {
  fprintf(2, "renombrar: cannot unlink %s\n", argv[1]);
  return 1;
}
fprintf(1, "'%s' renombrado a '%s'.\n", argv[1], argv[2]);
```

Cómo se usa:
```
renombrar viejo.txt nuevo.txt
  → 'viejo.txt' renombrado a 'nuevo.txt'.
```

### `copiar`
Hace una copia de un archivo. Le das el archivo original y el nombre de la copia. No te deja si el destino ya existe. Lee el archivo en bloques de `BUF_SIZE` (512 bytes) y los escribe en el nuevo. Si algo falla mientras copia, borra la copia incompleta para no dejarte archivos rotos.

```c
// De EAFITossh.c
char buf[BUF_SIZE];
int n;
while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
  if (write(dst_fd, buf, n) != n) {
    fprintf(2, "copiar: write error\n");
    close(src_fd);
    close(dst_fd);
    unlink(argv[2]);   // borra la copia incompleta
    return 1;
  }
}
fprintf(1, "'%s' copiado a '%s'.\n", argv[1], argv[2]);
```

Cómo se usa:
```
copiar original.txt copia.txt
  → 'original.txt' copiado a 'copia.txt'.
```

### `mover`
Mueve un archivo de un lugar a otro. Si el destino ya existe, no lo hace. Internamente funciona igual que `renombrar`: usa `link()` para crear el nuevo nombre y `unlink()` para borrar el viejo. La diferencia con `renombrar` es conceptual: `mover` se usa cuando quieres cambiar de carpeta, y `renombrar` cuando quieres cambiar el nombre dentro de la misma carpeta.

```c
// De EAFITossh.c
if (link(argv[1], argv[2]) < 0) {
  fprintf(2, "mover: cannot link %s to %s\n", argv[1], argv[2]);
  return 1;
}
if (unlink(argv[1]) < 0) {
  fprintf(2, "mover: cannot unlink %s\n", argv[1]);
  return 1;
}
fprintf(1, "'%s' movido a '%s'.\n", argv[1], argv[2]);
```

Cómo se usa:
```
mover archivo.txt carpeta/archivo.txt
  → 'archivo.txt' movido a 'carpeta/archivo.txt'.
```

### `buscar`
Busca un texto dentro de un archivo y te muestra las líneas donde lo encontró, junto con el número de línea. Es como un `grep` sencillo en español. Lee el archivo carácter por carácter con `read()` para armar cada línea, y luego usa `strstr()` para ver si el patrón aparece ahí.

Si no encuentra nada, te lo dice. Si encuentra, te muestra cada línea que coincide y al final te dice cuántas encontró.

```c
// De EAFITossh.c
while (read(fd, &c, 1) == 1) {
  if (c == '\n' || line_pos >= LINE_MAX - 1) {
    line[line_pos] = 0;
    lineno++;
    if (strstr(line, pattern) != 0) {
      fprintf(1, "  %d: %s\n", lineno, line);
      matches++;
    }
    line_pos = 0;
  } else {
    line[line_pos++] = c;
  }
}
```

Cómo se usa:
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
Te da información sobre un archivo: su tamaño, tipo (regular, directorio, dispositivo), número de links, número de inode, y si es un archivo de texto también te dice cuántas líneas, palabras y caracteres tiene.

Usa la llamada al sistema `stat()` para obtener la información del archivo. Para contar líneas, palabras y caracteres, abre el archivo y lo recorre carácter por carácter. Como el sistema de archivos no maneja permisos ni fechas de modificación, esos campos aparecen como `N/A`.

```c
// De EAFITossh.c
struct stat st;
stat(argv[1], &st);

fprintf(1, "Archivo:    %s\n", argv[1]);
fprintf(1, "Tamano:     %ld bytes\n", st.size);
// ...
fprintf(1, "Links:      %d\n", (int)st.nlink);
fprintf(1, "Inode:      %d\n", (int)st.ino);
fprintf(1, "Permisos:   N/A (xv6)\n");
fprintf(1, "Timestamps: N/A (xv6)\n");

// Cuenta líneas, palabras y caracteres
while (read(fd, &c, 1) == 1) {
  chars++;
  if (c == '\n') lines++;
  if (c == ' ' || c == '\t' || c == '\n') in_word = 0;
  else if (!in_word) { in_word = 1; words++; }
}
```

Cómo se usa:
```
estadisticas mi_archivo.txt
```

Salida:
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

Además de los comandos, la shell soporta operaciones del sistema de archivos. El parser entiende un árbol de structs (`execcmd`, `pipecmd`, `redircmd`, `listcmd`, `backcmd`) para representar y ejecutar combinaciones de comandos.

- **Tuberías (`|`)**: Encadena comandos para que la salida de uno sea la entrada del otro. Internamente crea un "pipe" entre los dos procesos usando `pipe()` y `fork()`. Ejemplo: `listar | buscar txt`
- **Redirección de salida (`>`)**: Manda la salida de un comando a un archivo en vez de a la pantalla. Si el archivo existe, lo sobreescribe. Ejemplo: `listar > archivos.txt`
- **Redirección de salida (append) (`>>`)**: Igual que `>`, pero agrega al final del archivo en vez de sobreescribir. Se simula buscando el final del archivo y escribiendo desde ahí. Ejemplo: `leer notas.txt >> log.txt`
- **Redirección de entrada (`<`)**: Hace que un comando lea desde un archivo en vez del teclado. Ejemplo: `buscar "hola" < archivo.txt`
- **Ejecución en segundo plano (`&`)**: Pone un comando a correr de fondo para que puedas seguir usando la shell mientras se ejecuta. Ejemplo: `sleep 10 &`
- **Comandos externos**: Todo lo que no sea un comando interno se busca como un programa en el sistema de archivos usando `exec()`. Ejemplo: `echo hola`

