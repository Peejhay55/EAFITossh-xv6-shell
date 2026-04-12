#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  void *va = (void*)0x40000000;
  
  printf("tmemro: mapeando pagina solo-lectura en %p\n", va);
  if(map_ro(va) < 0){
    printf("tmemro: map_ro failed\n");
    exit(1);
  }
  
  char *str = (char*)va;
  printf("tmemro: leyendo pagina leida... [%s]\n", str);
  
  printf("tmemro: intentando escribir en pagina RO, deberia fallar...\n");
  str[0] = 'X';
  
  printf("tmemro: ERROR esto no deberia imprimir!\n");
  exit(0);
}