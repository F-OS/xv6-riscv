#include "kernel/sysinfo.h"
#include "kernel/riscv.h"
#include "kernel/types.h"
#include "user/user.h"

//
// use sbrk() to count how many free physical memory pages there are.
//
int countfree() {
  uint64 sz0 = (uint64)sbrk(0);
  int n = 0;

  while (1) {
    if ((uint64)sbrk(PGSIZE) == 0xffffffffffffffff) {
      break;
    }
    n += PGSIZE;
  }
  
  if (kshare->freemem != 0) {
    printf("FAIL: there is no free mem, but sysinfo.freemem=%d\n",
           kshare->freemem);
    exit(1);
  }
  sbrk(-((uint64)sbrk(0) - sz0));
  return n;
}

void testmem() {
  struct sysinfo info;
  uint64 n = countfree();

  if (kshare->freemem != n) {
    printf("FAIL: free mem %d (bytes) instead of %d\n", kshare->freemem, n);
    exit(1);
  }

  if ((uint64)sbrk(PGSIZE) == 0xffffffffffffffff) {
    printf("sbrk failed");
    exit(1);
  }


  if (kshare->freemem != n - PGSIZE) {
    printf("FAIL: free mem %d (bytes) instead of %d\n", n - PGSIZE,
           kshare->freemem);
    exit(1);
  }

  if ((uint64)sbrk(-PGSIZE) == 0xffffffffffffffff) {
    printf("sbrk failed");
    exit(1);
  }

  if (kshare->freemem != n) {
    printf("FAIL: free mem %d (bytes) instead of %d\n", n, kshare->freemem);
    exit(1);
  }
}

void testcall() {
  struct sysinfo info;

  if (sysinfo(&info) < 0) {
    printf("FAIL: sysinfo failed\n");
    exit(1);
  }

  if (sysinfo((struct sysinfo *)0xeaeb0b5b00002f5e) != 0xffffffffffffffff) {
    printf("FAIL: sysinfo succeeded with bad argument\n");
    exit(1);
  }
}

void testproc() {
  struct sysinfo info;
  uint64 nproc;
  int status;
  int pid;

  nproc = kshare->nproc;

  pid = fork();
  if (pid < 0) {
    printf("sysinfotest: fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    if (kshare->nproc != nproc + 1) {
      printf("sysinfotest: FAIL nproc is %d instead of %d\n", kshare->nproc,
             nproc + 1);
      exit(1);
    }
    exit(0);
  }
  wait(&status);
  if (kshare->nproc != nproc) {
    printf("sysinfotest: FAIL nproc is %d instead of %d\n", kshare->nproc, nproc);
    exit(1);
  }
}

void testbad() {
  int pid = fork();
  int xstatus;

  if (pid < 0) {
    printf("sysinfotest: fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    exit(0);
  }
  wait(&xstatus);
  if (xstatus == -1) // kernel killed child?
    exit(0);
  else {
    printf("sysinfotest: testbad succeeded %d\n", xstatus);
    exit(xstatus);
  }
}

int main(int argc, char *argv[]) {
  printf("sysinfotest: start\n");
  testcall();
  testmem();
  testproc();
  printf("sysinfotest: OK\n");
  exit(0);
}