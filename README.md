# EAFITossh - xv6 Shell 

**Autor:** Pablo José Benítez Trujillo
**Clase:** Sistemas Operativos C2661-SI2004-4787

## Métodos que usa la shell
La shell `EAFITossh` implementa diversos comandos internos (built-ins) y soporta funcionalidades avanzadas de Unix:

### Comandos Internos
- `cd [ruta]`: Cambia el directorio de trabajo.
- `help` / `ayuda`: Muestra la lista de comandos disponibles.
- `exit` / `salir`: Finaliza la sesión de la shell.
- `listar [ruta]`: Muestra los archivos y carpetas en el directorio actual.
- `leer <archivo>`: Muestra el contenido de un archivo de texto.
- `tiempo`: Muestra la fecha y hora actual (formato 12h y AM/PM, ajustado para Colombia UTC-5).
- `calc <n1> <op> <n2>`: Calculadora básica para enteros (+, -, *, /, %).
- `crear <archivo>`: Crea un archivo nuevo vacío.
- `eliminar <archivo>`: Borra un archivo tras una confirmación del usuario.
- `renombrar <viejo> <nuevo>`: Cambia el nombre de un archivo.
- `copiar <origen> <destino>`: Duplica un archivo.
- `mover <origen> <destino>`: Traslada o renombra un archivo.
- `buscar <texto> <archivo>`: Busca coincidencias de una palabra en un archivo.
- `estadisticas <archivo>`: Muestra información detallada (tamaño, tipo, inodo) del archivo.

### Funcionalidades de Procesos
- **Redirección:** Soporte para `<` y `>`.
- **Tuberías (Pipes):** Uso de `|` para comunicar procesos.
- **Ejecución en segundo plano:** Uso de `&`.
- **Listas de comandos:** Ejecución secuencial usando `;`.

## Pre-requisitos
Para compilar y ejecutar xv6 y la shell EAFITossh, se requiere:
1. Una cadena de herramientas (toolchain) RISC-V "newlib" (por ejemplo, `riscv64-unknown-elf-gcc`).
2. QEMU configurado para la arquitectura `riscv64-softmmu`.

## Guía paso a paso para correr la shell EAFITossh
Siga estos pasos para iniciar la shell personalizada dentro del entorno de xv6:

1. **Compilar y arrancar xv6:**
   Abra una terminal en la raíz del proyecto y ejecute:
   ```bash
   make qemu
   ```
2. **Iniciar EAFITossh:**
   Una vez que xv6 haya cargado y aparezca el prompt original `$`, escriba el nombre del programa:
   ```bash
   EAFITossh
   ```
3. **Uso de la shell:**
   Ya dentro de `EAFITossh`, podrá ver el banner de bienvenida y empezar a usar los comandos descritos. Escriba `ayuda` para ver la lista en cualquier momento.

---

xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)).  See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6.

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by
Abhinavpatel00, Takahiro Aoyagi, Marcelo Arroyo, Hirbod Behnam, Silas
Boyd-Wickizer, Anton Burtsev, carlclone, Ian Chen, clivezeng, Dan
Cross, Cody Cutler, Mike CAT, Tej Chajed, Asami Doi,Wenyang Duan,
echtwerner, eyalz800, Nelson Elhage, Saar Ettinger, Alice Ferrazzi,
Nathaniel Filardo, flespark, Peter Froehlich, Yakir Goaron, Shivam
Handa, Matt Harvey, Bryan Henry, jaichenhengjie, Jim Huang, Matúš
Jókay, John Jolly, Alexander Kapshuk, Anders Kaseorg, kehao95,
Wolfgang Keller, Jungwoo Kim, Jonathan Kimmitt, Eddie Kohler, Vadim
Kolontsov, Austin Liew, l0stman, Pavan Maddamsetti, Imbar Marinescu,
Yandong Mao, Matan Shabtay, Hitoshi Mitake, Carmi Merimovich,
mes900903, Mark Morrissey, mtasm, Joel Nider, Hayato Ohhashi,
OptimisticSide, papparapa, phosphagos, Harry Porter, Greg Price, Zheng
qhuo, Quancheng, RayAndrew, Jude Rich, segfault, Ayan Shafqat, Eldar
Sehayek, Yongming Shen, Fumiya Shigemitsu, snoire, Taojie, Cam Tenny,
tyfkda, Warren Toomey, Stephen Tu, Alissa Tung, Rafael Ubal, unicornx,
Amane Uehara, Pablo Ventura, Luc Videau, Xi Wang, WaheedHafez, Keiichi
Watanabe, Lucas Wolf, Nicolas Wolovick, wxdao, Grant Wu, x653, Andy
Zhang, Jindong Zhang, Icenowy Zheng, ZhUyU1997, and Zou Chang Wei.

ERROR REPORTS

Please send errors and suggestions to Frans Kaashoek and Robert Morris
(kaashoek,rtm@mit.edu).  The main purpose of xv6 is as a teaching
operating system for MIT's 6.1810, so we are more interested in
simplifications and clarifications than new features.

BUILDING AND RUNNING XV6

You will need a RISC-V "newlib" tool chain from
https://github.com/riscv/riscv-gnu-toolchain, and qemu compiled for
riscv64-softmmu.  Once they are installed, and in your shell
search path, you can run "make qemu".
