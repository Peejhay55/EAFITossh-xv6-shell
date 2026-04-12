#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

struct {
  struct spinlock lock;
  char *page;
  int refs;
} shmstate;

void
shm_init(void)
{
  initlock(&shmstate.lock, "shmstate");
  shmstate.page = 0;
  shmstate.refs = 0;
}

void
shm_proc_exit(struct proc *p)
{
  acquire(&shmstate.lock);
  if(p->shm_attached){
    uvmunmap(p->pagetable, SHM_VA, 1, 0);
    p->shm_attached = 0;

    if(shmstate.refs < 1)
      panic("shm refs");
    shmstate.refs--;

    if(shmstate.refs == 0 && shmstate.page != 0){
      kfree(shmstate.page);
      shmstate.page = 0;
    }
  }
  release(&shmstate.lock);
}

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > SHM_VA)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// return seconds since Unix epoch from the Goldfish RTC
// that QEMU's virt machine provides at 0x101000.
uint64
sys_rtctime(void)
{
  // TIME_LOW at offset 0, TIME_HIGH at offset 4 (nanoseconds).
  volatile uint32 *rtc = (volatile uint32 *)RTC0;
  uint64 lo = rtc[0];   // reading TIME_LOW latches TIME_HIGH
  uint64 hi = rtc[1];
  uint64 ns = (hi << 32) | lo;
  return ns / 1000000000ULL;  // convert ns -> seconds
}

uint64
sys_shm_open(void)
{
  struct proc *p = myproc();

  acquire(&shmstate.lock);

  if(p->shm_attached){
    release(&shmstate.lock);
    return SHM_VA;
  }

  if(shmstate.page == 0){
    shmstate.page = kalloc();
    if(shmstate.page == 0){
      release(&shmstate.lock);
      return -1;
    }
    memset(shmstate.page, 0, PGSIZE);
  }

  if(mappages(p->pagetable, SHM_VA, PGSIZE, (uint64)shmstate.page,
              PTE_U | PTE_R | PTE_W) < 0){
    if(shmstate.refs == 0){
      kfree(shmstate.page);
      shmstate.page = 0;
    }
    release(&shmstate.lock);
    return -1;
  }

  p->shm_attached = 1;
  shmstate.refs++;

  release(&shmstate.lock);
  return SHM_VA;
}

uint64
sys_shm_close(void)
{
  shm_proc_exit(myproc());
  return 0;
}

// EAFITos: Syscall hello
uint64
sys_hello(void)
{
  printf("Hello from EAFITos kernel!\n");
  return 0;
}

// EAFITos: syscall strace (trace)
uint64
sys_trace(void)
{
  int mask;
  argint(0, &mask);
  myproc()->trace_mask = mask;
  return 0;
}

// EAFITos: syscall dumpvm
uint64
sys_dumpvm(void)
{
  vmprint(myproc()->pagetable);
  return 0;
}

// EAFITos: syscall map_ro
uint64
sys_map_ro(void)
{
  uint64 va;
  argaddr(0, &va);
  
  va = PGROUNDDOWN(va);
  char *mem = kalloc();
  if(mem == 0)
    return -1;
    
  memset(mem, 0, PGSIZE);
  memmove(mem, "Mensaje corto de prueba desde el kernel (solo lectura)", 54);
  
  struct proc *p = myproc();
  if(mappages(p->pagetable, va, PGSIZE, (uint64)mem, PTE_R | PTE_U) != 0){
    kfree(mem);
    return -1;
  }
  
  p->map_ro_va = va;
  return 0;
}
