#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main() {
  printf("Probando syscalls con argumentos validos e invalidos...\n\n");

  char buf[10];
  int fd;

  // Habilitar el trace de estas syscalls para ver los registros crudos y decodificados
  // read(5)  => 32
  // open(15) => 32768
  // write(16)=> 65536
  // hello(25)=> 33554432
  // dumpvm(27)=> 134217728
  trace(134217728 + 33554432 + 65536 + 32768 + 32);

  printf("--- 1. open() con punteros ---\n");
  fd = open("README", O_RDONLY); // Valido
  if(fd >= 0) close(fd);
  printf(">> Intentando open(0x0)...\n");
  int r1 = open((char*)0x0, O_RDONLY); // Invalido

  printf(">> Intentando open(0xffffffffffffffff)...\n");
  int r2 = open((char*)0xffffffffffffffff, O_RDONLY); // Invalido

  printf("\n--- 2. read() con punteros ---\n");
  fd = open("README", O_RDONLY);
  read(fd, buf, 10); // Valido
  
  printf(">> Intentando read() a 0x0...\n");
  read(fd, (void*)0x0, 10); // Invalido

  printf(">> Intentando read() a 0xffffffffffffffff...\n");
  read(fd, (void*)0xffffffffffffffff, 10); // Invalido
  if(fd >= 0) close(fd);

  printf("\n--- 3. write() con punteros ---\n");
  write(1, "hola!\n", 6); // Valido
  
  printf(">> Intentando write() desde 0x0...\n");
  write(1, (void*)0x0, 5); // Invalido

  printf(">> Intentando write() desde 0xffffffffffffffff...\n");
  write(1, (void*)0xffffffffffffffff, 5); // Invalido

  printf("\n--- 4. syscall hello() [sin argumentos] ---\n");
  hello();

  printf("\nTerminado con exito. Resultados invalidos: open(0x0)=%d, open(0xfff...)=%d\n", r1, r2);
  exit(0);
}