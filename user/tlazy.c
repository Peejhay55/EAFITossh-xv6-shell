#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/**
 * Programa user/tlazy.c
 * Prueba la implementación de Lazy Allocation.
 * Debe fallar con un Page Fault al intentar acceder a la memoria
 * que fue "reservada" virtualmente pero no físicamente.
 */

int
main(int argc, char *argv[])
{
  uint64 n_pages = 10;
  uint64 sz = n_pages * 4096;
  char *p = sbrk(sz);
  
  if (p == (char*)-1) {
    printf("tlazy: sbrk falló\n");
    exit(1);
  }

  printf("tlazy: Memoria reservada virtualmente (%d paginas) en %p\n", (int)n_pages, p);

  if (argc > 1 && argv[1][0] == 'r') {
    // 2. Acceso Aleatorio (Disperso)
    printf("tlazy: Acceso aleatorio/disperso...\n");
    for(int i = (int)n_pages - 1; i >= 0; i--){
      p[i * 4096] = 'r';
      printf("tlazy: Acceso a pagina %d completa\n", i);
    }
  } else {
    // 1. Acceso Secuencial
    printf("tlazy: Acceso secuencial...\n");
    for(int i = 0; i < (int)n_pages; i++){
      p[i * 4096] = 's';
      printf("tlazy: Acceso a pagina %d completa\n", i);
    }
  }

  printf("tlazy: Prueba finalizada. Revisa el log del kernel para ver los pf_count.\n");
  exit(0);
}
