#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

// ============================================================
//  Constantes
// ============================================================

#define EAFITOS_NAME    "EAFITos"
#define EAFITOS_VERSION "0.1.0"

#define MAXARGS    10
#define LINE_MAX   1024
#define BUF_SIZE   512

// Tipos de comandos que entiende la shell
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5

// Banderas para manejo de archivos
#define MODE_TRUNC  0x1000
#define MODE_APPEND 0x2000

// ============================================================
//  Estructuras del parser
// ============================================================

struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

// ============================================================
//  Estado de la shell
// ============================================================

struct shell_ctx {
  int running;
  int last_exit_status;
};

// Variable global para saber si la shell sigue corriendo
struct shell_ctx g_ctx;

// ============================================================
//  Prototipos de funciones
// ============================================================

int fork1(void);
void panic(char*);
struct cmd *parsecmd(char*);
void runcmd(struct cmd*) __attribute__((noreturn));

// Funciones del parser
struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);
struct cmd *parseredirs(struct cmd*, char**, char*);
struct cmd *parseblock(char**, char*);

// Funciones para crear nodos de comandos
struct cmd* execcmd(void);
struct cmd* redircmd(struct cmd*, char*, char*, int, int);
struct cmd* pipecmd(struct cmd*, struct cmd*);
struct cmd* listcmd(struct cmd*, struct cmd*);
struct cmd* backcmd(struct cmd*);

// ============================================================
//  Funciones para manejar texto
// ============================================================

// Mira si un caracter es un espacio o algo parecido
int
xv6_isspace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v';
}

// Compara dos textos con un limite
int
strncmp(const char *a, const char *b, uint n)
{
  while (n > 0 && *a && *b && *a == *b) {
    a++;
    b++;
    n--;
  }
  if (n == 0)
    return 0;
  return (uchar)*a - (uchar)*b;
}

// Busca un texto dentro de otro
char*
strstr(const char *haystack, const char *needle)
{
  if (!*needle)
    return (char*)haystack;
  for (; *haystack; haystack++) {
    const char *h = haystack;
    const char *n = needle;
    while (*h && *n && *h == *n) {
      h++;
      n++;
    }
    if (!*n)
      return (char*)haystack;
  }
  return 0;
}

// Pega un texto al final de otro
char*
strcat(char *dst, const char *src)
{
  char *d = dst;
  while (*d) d++;
  while (*src) *d++ = *src++;
  *d = 0;
  return dst;
}

// Quita los espacios al principio y al final
char*
str_trim(char *str)
{
  while (xv6_isspace(*str))
    str++;
  if (*str == 0)
    return str;

  char *end = str + strlen(str) - 1;
  while (end > str && xv6_isspace(*end)) {
    *end = 0;
    end--;
  }
  return str;
}

// ============================================================
//  Funciones para mostrar errores
// ============================================================

void
shell_error(char *msg)
{
  fprintf(2, "%s: %s\n", EAFITOS_NAME, msg);
}

void
shell_error2(char *msg, char *arg)
{
  fprintf(2, "%s: %s %s\n", EAFITOS_NAME, msg, arg);
}

void
shell_warn(char *msg)
{
  fprintf(2, "%s [warn]: %s\n", EAFITOS_NAME, msg);
}

// ============================================================
//  Mensaje de bienvenida y prompt
// ============================================================

// Muestra el nombre de la shell y el signo de pesos
void
print_prompt(void)
{
  fprintf(1, "%s$ ", EAFITOS_NAME);
}

// Un dibujo bonito para cuando empieza la shell
void
print_banner(void)
{
  fprintf(1, "\n");
  fprintf(1, "    _________    ________________          \n");
  fprintf(1, "   / ____/   |  / ____/  _/_  __/___  _____\n");
  fprintf(1, "  / __/ / /| | / /_   / /  / / / __ \\/ ___/\n");
  fprintf(1, " / /___/ ___ |/ __/ _/ /  / / / /_/ (__  ) \n");
  fprintf(1, "/_____/_/  |_/_/   /___/ /_/  \\____/____/  \n");
  fprintf(1, "\n");
  fprintf(1, "  v%s -- escribe 'ayuda' para ver una lista de comandos.\n\n",
          EAFITOS_VERSION);
}

// Rellena con espacios para que todo se vea alineado
void
print_padded(int fd, char *s, int width)
{
  int len = strlen(s);
  fprintf(fd, "%s", s);
  while (len < width) {
    write(fd, " ", 1);
    len++;
  }
}

// ============================================================
//  Lista de comandos que ya vienen en la shell
// ============================================================

struct help_entry {
  char *name;
  char *desc;
};

struct help_entry help_table[] = {
  { "cd",           "Change the working directory"              },
  { "exit",         "Exit the shell"                            },
  { "help",         "Show available built-in commands"          },
  { "listar",       "Muestra contenido del directorio actual"   },
  { "leer",         "Muestra contenido de un archivo de texto"  },
  { "tiempo",       "Muestra la fecha y hora actual"            },
  { "calc",         "Calculadora basica (calc N1 op N2)"        },
  { "ayuda",        "Muestra lista de comandos"                 },
  { "salir",        "Termina la shell"                          },
  { "crear",        "Crea archivo vacio"                        },
  { "eliminar",     "Elimina archivo con confirmacion"          },
  { "renombrar",    "Renombra archivo"                          },
  { "copiar",       "Copia archivo"                             },
  { "mover",        "Mueve archivo"                             },
  { "buscar",       "Busca texto en archivo"                    },
  { "estadisticas", "Estadisticas del archivo"                  },
  { 0, 0 },
};

// ============================================================
//  Funciones de los comandos internos
// ============================================================

// Comandos basicos

// Para cambiarse de carpeta
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

// Para cerrar la shell normal
int
builtin_exit_cmd(char **argv, int argc)
{
  if (argc > 1)
    g_ctx.last_exit_status = atoi(argv[1]);
  g_ctx.running = 0;
  return g_ctx.last_exit_status;
}

// Para cerrar la shell con estilo
int
builtin_salir(char **argv, int argc)
{
  fprintf(1, "Hasta luego!\n");
  return builtin_exit_cmd(argv, argc);
}

// Muestra que comandos se pueden usar en ingles
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

// Muestra que comandos se pueden usar en espanol
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

// Herramientas utiles en espanol

// Muestra lo que hay en una carpeta
int
builtin_listar(char **argv, int argc)
{
  char *path = (argc > 1) ? argv[1] : ".";

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "listar: cannot open %s\n", path);
    return 1;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    fprintf(2, "listar: cannot stat %s\n", path);
    close(fd);
    return 1;
  }

  if (st.type != T_DIR) {
    fprintf(2, "listar: %s is not a directory\n", path);
    close(fd);
    return 1;
  }

  struct dirent de;
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;

    // Ignora los archivos que empiezan con punto
    if (de.name[0] == '.')
      continue;

    // Le pone el fin de cadena al nombre
    char name[DIRSIZ + 1];
    int i;
    for (i = 0; i < DIRSIZ && de.name[i]; i++)
      name[i] = de.name[i];
    name[i] = 0;

    // Junta la ruta para poder ver los archivos
    char fullpath[DIRSIZ + 128];
    strcpy(fullpath, path);
    strcat(fullpath, "/");
    strcat(fullpath, name);

    struct stat entry_st;
    if (stat(fullpath, &entry_st) >= 0 && entry_st.type == T_DIR) {
      fprintf(1, "  %s/\n", name);
    } else {
      fprintf(1, "  %s\n", name);
    }
  }

  close(fd);
  return 0;
}

// Muestra lo que tiene un archivo de texto
int
builtin_leer(char **argv, int argc)
{
  if (argc < 2) {
    fprintf(2, "leer: uso: leer <archivo>\n");
    return 1;
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(2, "leer: cannot open %s\n", argv[1]);
    return 1;
  }

  char buf[BUF_SIZE];
  int n;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    write(1, buf, n);
  }

  close(fd);
  return 0;
}

// Muestra la fecha y la hora actual
int
builtin_tiempo(void)
{
  // Ajuste de hora para Colombia
  int utc_offset = -5;

  int epoch = rtctime();
  epoch += utc_offset * 3600;

  // Convierte el tiempo total en horas, minutos y segundos
  int secs_in_day = epoch % 86400;
  if (secs_in_day < 0)
    secs_in_day += 86400;

  int hours   = secs_in_day / 3600;
  int minutes = (secs_in_day % 3600) / 60;
  int seconds = secs_in_day % 60;

  // Formato de 12 horas con AM/PM
  char *ampm = "AM";
  int h12 = hours;
  if (h12 >= 12) {
    ampm = "PM";
    if (h12 > 12) h12 -= 12;
  }
  if (h12 == 0) h12 = 12;

  // Calcula la fecha desde 1970
  int days = epoch / 86400;
  if (epoch < 0 && epoch % 86400 != 0)
    days--;

  // Algoritmo loco para sacar la fecha
  days += 719468;
  int era = (days >= 0 ? days : days - 146096) / 146097;
  int doe = days - era * 146097;
  int yoe = (doe - doe/1461 + doe/36524 - doe/146096) / 365;
  int y   = yoe + era * 400;
  int doy = doe - (365*yoe + yoe/4 - yoe/100);
  int mp  = (5*doy + 2) / 153;
  int d   = doy - (153*mp + 2)/5 + 1;
  int m   = mp + (mp < 10 ? 3 : -9);
  if (m <= 2) y++;

  char *meses[] = { "", "Ene", "Feb", "Mar", "Abr", "May", "Jun",
                        "Jul", "Ago", "Sep", "Oct", "Nov", "Dic" };

  fprintf(1, "Fecha: %s %d, %d\n", meses[m], d, y);
  fprintf(1, "Hora:  %d:%d%d:%d%d %s (UTC%d)\n",
          h12, minutes/10, minutes%10, seconds/10, seconds%10,
          ampm, utc_offset);

  return 0;
}

// Una calculadora sencilla para numeros enteros
int
builtin_calc(char **argv, int argc)
{
  if (argc != 4) {
    fprintf(2, "calc: uso: calc <num1> <operador> <num2>\n");
    fprintf(2, "  operadores: + - * / %%\n");
    return 1;
  }

  int a = atoi(argv[1]);
  int b = atoi(argv[3]);
  char *op = argv[2];
  int result;

  if (strcmp(op, "+") == 0) {
    result = a + b;
  } else if (strcmp(op, "-") == 0) {
    result = a - b;
  } else if (strcmp(op, "*") == 0) {
    result = a * b;
  } else if (strcmp(op, "/") == 0) {
    if (b == 0) {
      fprintf(2, "calc: division por cero\n");
      return 1;
    }
    result = a / b;
  } else if (strcmp(op, "%") == 0 || strcmp(op, "mod") == 0) {
    if (b == 0) {
      fprintf(2, "calc: division por cero\n");
      return 1;
    }
    result = a % b;
  } else {
    fprintf(2, "calc: operador desconocido '%s'\n", op);
    fprintf(2, "  operadores validos: + - * / %% mod\n");
    return 1;
  }

  fprintf(1, "%d\n", result);
  return 0;
}

// Comandos para manejar archivos

// Crea un archivo nuevo y vacio
int
builtin_crear(char **argv, int argc)
{
  if (argc < 2) {
    fprintf(2, "crear: uso: crear <archivo>\n");
    return 1;
  }

  // No deja sobreescribir si ya existe
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
  return 0;
}

// Borra un archivo pero pregunta primero
int
builtin_eliminar(char **argv, int argc)
{
  if (argc < 2) {
    fprintf(2, "eliminar: uso: eliminar <archivo>\n");
    return 1;
  }

  // Mira si el archivo de verdad existe
  struct stat st;
  if (stat(argv[1], &st) < 0) {
    fprintf(2, "eliminar: '%s' no existe\n", argv[1]);
    return 1;
  }

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
  return 0;
}

// Cambia el nombre de un archivo
int
builtin_renombrar(char **argv, int argc)
{
  if (argc < 3) {
    fprintf(2, "renombrar: uso: renombrar <viejo> <nuevo>\n");
    return 1;
  }

  struct stat st;
  if (stat(argv[1], &st) < 0) {
    fprintf(2, "renombrar: '%s' no existe\n", argv[1]);
    return 1;
  }

  if (stat(argv[2], &st) >= 0) {
    fprintf(2, "renombrar: el destino '%s' ya existe\n", argv[2]);
    return 1;
  }

  if (link(argv[1], argv[2]) < 0) {
    fprintf(2, "renombrar: cannot link %s to %s\n", argv[1], argv[2]);
    return 1;
  }
  if (unlink(argv[1]) < 0) {
    fprintf(2, "renombrar: cannot unlink %s (link created as %s)\n",
            argv[1], argv[2]);
    return 1;
  }

  fprintf(1, "'%s' renombrado a '%s'.\n", argv[1], argv[2]);
  return 0;
}

// Saca una copia de un archivo
int
builtin_copiar(char **argv, int argc)
{
  if (argc < 3) {
    fprintf(2, "copiar: uso: copiar <origen> <destino>\n");
    return 1;
  }

  int src_fd = open(argv[1], O_RDONLY);
  if (src_fd < 0) {
    fprintf(2, "copiar: cannot open %s\n", argv[1]);
    return 1;
  }

  // No deja copiar si el destino ya existe
  struct stat st;
  if (stat(argv[2], &st) >= 0) {
    fprintf(2, "copiar: el destino '%s' ya existe\n", argv[2]);
    close(src_fd);
    return 1;
  }

  int dst_fd = open(argv[2], O_CREATE | O_WRONLY);
  if (dst_fd < 0) {
    fprintf(2, "copiar: cannot create %s\n", argv[2]);
    close(src_fd);
    return 1;
  }

  char buf[BUF_SIZE];
  int n;
  while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
    if (write(dst_fd, buf, n) != n) {
      fprintf(2, "copiar: write error\n");
      close(src_fd);
      close(dst_fd);
      unlink(argv[2]);
      return 1;
    }
  }

  close(src_fd);
  close(dst_fd);

  fprintf(1, "'%s' copiado a '%s'.\n", argv[1], argv[2]);
  return 0;
}

// Mueve un archivo de un lado a otro
int
builtin_mover(char **argv, int argc)
{
  if (argc < 3) {
    fprintf(2, "mover: uso: mover <origen> <destino>\n");
    return 1;
  }

  struct stat st;
  if (stat(argv[1], &st) < 0) {
    fprintf(2, "mover: '%s' no existe\n", argv[1]);
    return 1;
  }

  if (stat(argv[2], &st) >= 0) {
    fprintf(2, "mover: el destino '%s' ya existe\n", argv[2]);
    return 1;
  }

  if (link(argv[1], argv[2]) < 0) {
    fprintf(2, "mover: cannot link %s to %s\n", argv[1], argv[2]);
    return 1;
  }
  if (unlink(argv[1]) < 0) {
    fprintf(2, "mover: cannot unlink %s (link created as %s)\n",
            argv[1], argv[2]);
    return 1;
  }

  fprintf(1, "'%s' movido a '%s'.\n", argv[1], argv[2]);
  return 0;
}

// Busca una palabra dentro de un archivo
int
builtin_buscar(char **argv, int argc)
{
  if (argc < 3) {
    fprintf(2, "buscar: uso: buscar <texto> <archivo>\n");
    return 1;
  }

  char *pattern = argv[1];
  char *filename = argv[2];

  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "buscar: cannot open %s\n", filename);
    return 1;
  }

  // Lee linea por linea buscando el texto
  char line[LINE_MAX];
  int lineno = 0;
  int matches = 0;
  int line_pos = 0;
  char c;

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
  // No se olvida de la ultima linea
  if (line_pos > 0) {
    line[line_pos] = 0;
    lineno++;
    if (strstr(line, pattern) != 0) {
      fprintf(1, "  %d: %s\n", lineno, line);
      matches++;
    }
  }

  close(fd);

  if (matches == 0) {
    fprintf(1, "No se encontro '%s' en '%s'.\n", pattern, filename);
  } else {
    fprintf(1, "\n%d coincidencia(s) encontrada(s).\n", matches);
  }
  return 0;
}

// Muestra informacion detallada del archivo
int
builtin_estadisticas(char **argv, int argc)
{
  if (argc < 2) {
    fprintf(2, "estadisticas: uso: estadisticas <archivo>\n");
    return 1;
  }

  struct stat st;
  if (stat(argv[1], &st) < 0) {
    fprintf(2, "estadisticas: cannot stat %s\n", argv[1]);
    return 1;
  }

  fprintf(1, "Archivo:    %s\n", argv[1]);
  fprintf(1, "Tamano:     %ld bytes\n", st.size);

  // Mira si es un archivo, carpeta o dispositivo
  char *tipo = "desconocido";
  if (st.type == T_FILE)        tipo = "archivo regular";
  else if (st.type == T_DIR)    tipo = "directorio";
  else if (st.type == T_DEVICE) tipo = "dispositivo";
  fprintf(1, "Tipo:       %s\n", tipo);

  fprintf(1, "Links:      %d\n", (int)st.nlink);
  fprintf(1, "Inode:      %d\n", (int)st.ino);

  // xv6 no guarda permisos ni fechas
  fprintf(1, "Permisos:   N/A (xv6)\n");
  fprintf(1, "Timestamps: N/A (xv6)\n");

  // Cuenta lineas, palabras y letras
  if (st.type == T_FILE) {
    int fd = open(argv[1], O_RDONLY);
    if (fd >= 0) {
      int lines = 0, words = 0, chars = 0;
      int in_word = 0;
      char c;

      while (read(fd, &c, 1) == 1) {
        chars++;
        if (c == '\n') lines++;
        if (c == ' ' || c == '\t' || c == '\n') {
          in_word = 0;
        } else if (!in_word) {
          in_word = 1;
          words++;
        }
      }
      close(fd);

      fprintf(1, "Lineas:     %d\n", lines);
      fprintf(1, "Palabras:   %d\n", words);
      fprintf(1, "Caracteres: %d\n", chars);
    }
  }

  return 0;
}

// Decide si el comando es interno o externo
int
handle_builtin(char *cmd_line)
{
  char *argv[MAXARGS + 1];
  int argc = 0;

  char *p = cmd_line;
  while (*p && argc < MAXARGS) {
    while (*p && xv6_isspace(*p)) p++;
    if (*p == 0) break;

    argv[argc++] = p;

    while (*p && !xv6_isspace(*p)) p++;
    if (*p) { *p = 0; p++; }
  }

  if (argc == 0) return 0;
  argv[argc] = 0;

  // Llama a la funcion que toca segun el comando
  if (strcmp(argv[0], "cd") == 0) {
    g_ctx.last_exit_status = builtin_cd(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "exit") == 0) {
    g_ctx.last_exit_status = builtin_exit_cmd(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "salir") == 0) {
    g_ctx.last_exit_status = builtin_salir(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "help") == 0) {
    g_ctx.last_exit_status = builtin_help();
    return 1;
  }
  if (strcmp(argv[0], "ayuda") == 0) {
    g_ctx.last_exit_status = builtin_ayuda();
    return 1;
  }
  if (strcmp(argv[0], "listar") == 0) {
    g_ctx.last_exit_status = builtin_listar(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "leer") == 0) {
    g_ctx.last_exit_status = builtin_leer(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "tiempo") == 0) {
    g_ctx.last_exit_status = builtin_tiempo();
    return 1;
  }
  if (strcmp(argv[0], "calc") == 0) {
    g_ctx.last_exit_status = builtin_calc(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "crear") == 0) {
    g_ctx.last_exit_status = builtin_crear(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "eliminar") == 0) {
    g_ctx.last_exit_status = builtin_eliminar(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "renombrar") == 0) {
    g_ctx.last_exit_status = builtin_renombrar(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "copiar") == 0) {
    g_ctx.last_exit_status = builtin_copiar(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "mover") == 0) {
    g_ctx.last_exit_status = builtin_mover(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "buscar") == 0) {
    g_ctx.last_exit_status = builtin_buscar(argv, argc);
    return 1;
  }
  if (strcmp(argv[0], "estadisticas") == 0) {
    g_ctx.last_exit_status = builtin_estadisticas(argv, argc);
    return 1;
  }

  // Si no es un comando interno, devuelve 0
  return 0;
}

// Revisa si el comando es complejo (tiene |, ; o &)
int
is_complex_command(char *cmd)
{
  while (*cmd) {
    if (*cmd == '|' || *cmd == ';' || *cmd == '&')
      return 1;
    cmd++;
  }
  return 0;
}

// Ejecuta los comandos usando la logica de xv6
void
runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit(1);

  switch(cmd->type){
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit(1);
    exec(ecmd->argv[0], ecmd->argv);
    fprintf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    close(rcmd->fd);

    // Saca nuestras banderas de modo
    {
      int mode = rcmd->mode;
      int custom = mode & (MODE_TRUNC | MODE_APPEND);
      mode &= ~(MODE_TRUNC | MODE_APPEND);

      // Borra el archivo para que parezca que se trunco
      if(custom & MODE_TRUNC)
        unlink(rcmd->file);

      if(open(rcmd->file, mode) < 0){
        fprintf(2, "open %s failed\n", rcmd->file);
        exit(1);
      }

      // Se va al final del archivo para escribir
      if(custom & MODE_APPEND){
        char tmpbuf[BUF_SIZE];
        while(read(rcmd->fd, tmpbuf, sizeof(tmpbuf)) > 0)
          ;
      }
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    if(fork1() == 0)
      runcmd(lcmd->left);
    wait(0);
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p) < 0)
      panic("pipe");
    // Lado izquierdo: manda la salida al tubo
    if(fork1() == 0){
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    // Lado derecho: recibe la entrada desde el tubo
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait(0);
    wait(0);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    if(fork1() == 0)
      runcmd(bcmd->cmd);
    // Los procesos en el fondo corren solos
    break;
  }
  exit(0);
}

// Lee lo que el usuario escribe
int
getcmd(char *buf, int nbuf)
{
  print_prompt();
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

// El corazon de la shell
int
main(void)
{
  static char buf[LINE_MAX];
  static char builtin_buf[LINE_MAX];  // Copia el comando para no danar el original
  int fd;

  // Configura el estado inicial
  g_ctx.running = 1;
  g_ctx.last_exit_status = 0;

  // Se asegura de que la consola este abierta
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  // Muestra el logo al empezar
  print_banner();

  // Bucle infinito para recibir comandos
  while(g_ctx.running && getcmd(buf, sizeof(buf)) >= 0){
    char *cmd = buf;

    // Ignora los espacios al principio
    while (xv6_isspace(*cmd))
      cmd++;

    // Ignora si no escribiste nada
    if (*cmd == '\n' || *cmd == 0)
      continue;

    // Quita el salto de linea del final
    int len = strlen(cmd);
    if (len > 0 && cmd[len - 1] == '\n')
      cmd[len - 1] = 0;

    // Si es un comando simple, primero mira si es interno
    if (!is_complex_command(cmd)) {
      // Copia el comando para no danar el original
      strcpy(builtin_buf, cmd);
      if (handle_builtin(builtin_buf))
        continue;
    }

    // Si es externo o complejo, lo ejecuta aparte
    if(fork1() == 0)
      runcmd(parsecmd(cmd));
    wait(0);
  }
  exit(0);
}

// Funciones de ayuda general

void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(1);
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

// Funciones para armar el arbol de comandos

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd*)cmd;
}

// El motor que entiende lo que escribes

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;

  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while(peek(ps, es, "&")){
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if(peek(ps, es, ";")){
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch(tok){
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      // Bandera para borrar el archivo al abrir
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE|MODE_TRUNC, 1);
      break;
    case '+':  // >>
      // Bandera para escribir al final
      cmd = redircmd(cmd, q, eq, O_RDWR|O_CREATE|MODE_APPEND, 1);
      break;
    }
  }
  return cmd;
}

struct cmd*
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if(!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if(!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if(peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|)&;")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if(argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// Le pone el cero final a todos los textos
struct cmd*
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    return 0;

  switch(cmd->type){
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}
