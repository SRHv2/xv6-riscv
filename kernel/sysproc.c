#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#ifdef PGTBL_SOL
#include "riscv.h"
#endif
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
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

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > UTOP)
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
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_vmprint(void)
{
  struct proc *p;

  p = myproc();
  vmprint(p->pagetable);
  return 0;
}
#endif

#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  uint64 start;
  uint64 addr;
  uint64 dest;
  int npage;

  struct proc *p = myproc();

  // Get the system call arguments.
  argaddr(0, &start);
  argint(1, &npage);
  argaddr(2, &dest);

  // Validate the number of pages.
  if (npage < 1 || npage > 256)
    return -1;

  // Temporary buffer: 256 pages = 32 bytes.
  char accessbits[32];
  memset(accessbits, 0, sizeof(accessbits));

  // Examine each requested page.
  for (int i = 0; i < npage; i++) {

    addr = start + (uint64)i * PGSIZE;

    // Check for address overflow or invalid virtual address.
    if (addr < start || addr >= MAXVA)
      return -1;

    // Find the page-table entry.
    pte_t *pte = walk(p->pagetable, addr, 0);

    // Ensure that the page is mapped and user-accessible.
    if (pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_U) == 0)
      return -1;

    // Check whether the page has been accessed.
    if (*pte & PTE_A) {

      // Set the corresponding bit in our result.
      accessbits[i / 8] |= (1 << (i % 8));

      // Clear the accessed bit for future tracking.
      *pte &= ~PTE_A;
    }
  }

  // Copy the result back to the user's address space.
  if (copyout(p->pagetable, p->sz, dest,
              accessbits, (npage + 7) / 8) < 0)
    return -1;

  return 0;
}
#endif

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

#ifdef LAB_LOCK
uint64
sys_cpupin(void)
{
  struct proc *p = myproc();
  int cpu;

  argint(0, &cpu);
  if (cpu < 0 || cpu >= NCPU)
    return -1;
  acquire(&p->lock);
  p->pincpu = &cpus[cpu];
  release(&p->lock);
  return 0;
}
#endif
