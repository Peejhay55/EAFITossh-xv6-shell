#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/**
 * Programa user/tmmap_sim.c
 * Prueba la simulación de mmap (mapzero).
 * Reserva una región virtual y accede a ella para ver cómo el kernel
 * la inicializa automáticamente con 'A'.
 */

int
main(int argc, char *argv[])
{
  int size = 4096 * 2; // 2 páginas
  printf("tmmap_sim: Solicitando mapzero de %d bytes...\n", size);

  uint64 addr = mapzero(size);
  if (addr == (uint64)-1) {
    printf("tmmap_sim: mapzero falló\n");
    exit(1);
  }

  char *p = (char *)addr;
  printf("tmmap_sim: Región mapeada en %p\n", p);

  // Acceso gradual
  printf("tmmap_sim: Accediendo a la primera página...\n");
  char c1 = p[0];
  printf("tmmap_sim: Contenido en p[0]: '%c' (esperado: 'A')\n", c1);

  printf("tmmap_sim: Accediendo a la segunda página...\n");
  char c2 = p[4096];
  printf("tmmap_sim: Contenido en p[4096]: '%c' (esperado: 'A')\n", c2);

  if (c1 == 'A' && c2 == 'A') {
    printf("tmmap_sim: EXITO! La simulación de mmap funciona correctamente.\n");
  } else {
    printf("tmmap_sim: ERROR! El contenido no es el esperado.\n");
  }

  exit(0);
}
