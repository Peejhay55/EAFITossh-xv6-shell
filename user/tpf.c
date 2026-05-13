#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/**
 * Programa user/tpf.c
 * Intenta acceder a direcciones de memoria no mapeadas para provocar Page Faults.
 */

int
main(int argc, char *argv[])
{
    printf("tpf: Iniciando pruebas de Page Fault...\n");

    if (argc < 2) {
        printf("Uso: tpf [load|store]\n");
        exit(0);
    }

    if (argv[1][0] == 'l') {
        // i. Intento de Load Page Fault (Lectura)
        // Accediendo a una dirección arbitraria no mapeada (0x4000)
        printf("tpf: Intentando lectura en 0x4000 (Load Page Fault)...\n");
        volatile char *ptr = (char *)0x4000;
        char val = *ptr;
        printf("tpf: Leido %d (esto no deberia ejecutarse)\n", val);
    } else if (argv[1][0] == 's') {
        // ii. Intento de Store Page Fault (Escritura)
        // Accediendo a una dirección arbitraria no mapeada (0x8000)
        printf("tpf: Intentando escritura en 0x8000 (Store Page Fault)...\n");
        volatile char *ptr = (char *)0x8000;
        *ptr = 1;
        printf("tpf: Escrito (esto no deberia ejecutarse)\n");
    } else {
        printf("Argumento no reconocido: %s\n", argv[1]);
    }

    exit(0);
}
