#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  int pid = getpid();
  printf("\n=== tdumpvm [%d]: Initial page table ===\n", pid);
  dumpvm();

  printf("\n=== tdumpvm [%d]: Allocating memory (malloc 4096 bytes) ===\n", pid);
  char *mem = (char *)malloc(4096); // allocating 1 page
  if (mem) {
    mem[0] = 'X'; // Modify it to ensure it is mapped
  }

  printf("\n=== tdumpvm [%d]: Updated page table ===\n", pid);
  dumpvm();

  exit(0);
}