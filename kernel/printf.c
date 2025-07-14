//
// formatted console output -- printf, panic.
//

#include "string.h"
#include <stdarg.h>

#include "console.h"
#include "kernel/param.h"
#include "kernel/proc.h"
#include "kernel/sbi.h"
#include "spinlock.h"
#include "types.h"

volatile int panicked = -1;

static char digits[] = "0123456789abcdef";

// Print to the console.
char printbuf[NCPU][256] = {}; // buffer for printf
int bufpos[NCPU] = {0};        // current position in the buffer

void bufwrite(int c) {
  uint64 cpu_id = cpuid();
  if (bufpos[cpu_id] < sizeof(printbuf) - 1) {
    printbuf[cpu_id][bufpos[cpu_id]++] = c;
    printbuf[cpu_id][bufpos[cpu_id]] = '\0';
  } else {
    // Flush
    printbuf[cpu_id][bufpos[cpu_id]] = '\0';
    sbi_debug_console_write(printbuf[cpu_id]);
    bufpos[cpu_id] = 0; // Reset buffer position
  }
}

void bufflush(void) {
  uint64 cpu_id = cpuid();
  if (bufpos[cpu_id] > 0) {
    printbuf[cpu_id][bufpos[cpu_id]] = '\0';
    sbi_debug_console_write(printbuf[cpu_id]);
    bufpos[cpu_id] = 0;
  }
}

static void printint(long long xx, uint base, int sign) {
  char buf[16];
  int i = 0;
  unsigned long long x = 0;

  if (sign && (sign = (xx < 0))) {
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);

  if (sign) {
    buf[i++] = '-';
  }

  while (--i >= 0) {
    bufwrite(buf[i]);
  }
}

static void printptr(uint64 x) {
  bufwrite('0');
  bufwrite('x');
  for (uint i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4) {
    bufwrite(digits[x >> (sizeof(uint64) * 8 - 4)]);
  }
}

int vprintf(const char *fmt, va_list ap) {
  push_off();
  int i = 0;
  int cx = 0;
  int c0 = 0;
  int c1 = 0;
  int c2 = 0;
  const char *s = NULL;

  for (i = 0; (cx = fmt[i] & 0xff) != 0; i++) {
    if (cx != '%') {
      bufwrite(cx);
      continue;
    }
    i++;
    c0 = fmt[i + 0] & 0xff;
    c1 = c2 = 0;
    if (c0) {
      c1 = fmt[i + 1] & 0xff;
    }
    if (c1) {
      c2 = fmt[i + 2] & 0xff;
    }
    if (c0 == 'd') {
      printint(va_arg(ap, int), 10, 1);
    } else if (c0 == 'l' && c1 == 'd') {
      printint(va_arg(ap, uint64), 10, 1);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'd') {
      printint(va_arg(ap, uint64), 10, 1);
      i += 2;
    } else if (c0 == 'u') {
      printint(va_arg(ap, int), 10, 0);
    } else if (c0 == 'l' && c1 == 'u') {
      printint(va_arg(ap, uint64), 10, 0);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'u') {
      printint(va_arg(ap, uint64), 10, 0);
      i += 2;
    } else if (c0 == 'x') {
      printint(va_arg(ap, int), 16, 0);
    } else if (c0 == 'l' && c1 == 'x') {
      printint(va_arg(ap, uint64), 16, 0);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'x') {
      printint(va_arg(ap, uint64), 16, 0);
      i += 2;
    } else if (c0 == 'p') {
      printptr(va_arg(ap, uint64));
    } else if (c0 == 's') {
      if ((s = va_arg(ap, char *)) == 0) {
        s = "(null)";
      }
      for (; *s; s++) {
        bufwrite(*s);
      }
    } else if (c0 == '%') {
      bufwrite('%');
    } else if (c0 == 0) {
      break;
    } else {
      // Print unknown % sequence to draw attention.
      bufwrite('%');
      bufwrite(c0);
    }

#if 0
    switch(c){
    case 'd':
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      printptr(va_arg(ap, uint64));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
#endif
  }
  bufflush();

  pop_off();

  return 0;
}

int printf(const char *fmt, ...) {
  va_list ap;
  int ret;
  if (panicked >= 0) {
    int whoami = cpuid();
    if (whoami != panicked) {
      // Only the first CPU to panic will print.
      return 0;
    }
  }

  va_start(ap, fmt);
  ret = vprintf(fmt, ap);
  va_end(ap);

  return ret;
}

void putstr(const char *s) {
  if (s == 0) {
    s = "(null)";
  }
  if (panicked >= 0) {
    int whoami = cpuid();
    if (whoami != panicked) {
      // Only the first CPU to panic will print.
      return;
    }
  }
  sbi_debug_console_write(s);
}

extern char end[];
struct symbol {
  uint64 addr;   // symbol address
  char name[64]; // NUL‑terminated symbol name
} __attribute__((packed));
struct symbols {
  char magic[5];
  int count;             // number of symbols
  struct symbol list[0]; // followed immediately by `count` entries
} __attribute__((packed));

extern uint8 kernel_symbols[];
extern uint64 kernel_symbols_size;

struct symbol *next_symbol(struct symbol *sym) {
  if (sym == NULL) {
    return NULL; // No symbol to start with
  }
  struct symbols *syms = (struct symbols *)kernel_symbols;
  if (sym >= syms->list && sym < syms->list + syms->count) {
    // Return the next symbol in the list
    return (struct symbol *)((char *)sym + sizeof(struct symbol) +
                             strlen(sym->name) + 1);
  }
  return NULL; // Out of bounds
}

const char *lookup_symbol(uint64 addr) {
  if (addr < 0x80000000 || addr >= (uint64)end) {
    return NULL; // Address is outside the kernel range
  }
  /* 2) Make sure we actually have symbols */
  struct symbols *syms = (struct symbols *)kernel_symbols;
  if (syms->magic[0] != 'K' || syms->magic[1] != 'S' || syms->magic[2] != 'Y' ||
      syms->magic[3] != 'M' || syms->magic[4] != '\0' || syms->count <= 0 ||
      kernel_symbols_size < sizeof(struct symbols)) {
    return NULL; // Invalid symbols or no symbols
  }

  /* 3) Binary‐search for the highest symbol <= addr */
  int lo = 0;
  int hi = syms->count - 1;
  int best = -1;

  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    uint64 sym_addr = syms->list[mid].addr;

    if (sym_addr == addr) {
      best = mid;
      break; // exact hit!
    } else if (sym_addr < addr) {
      best = mid;   // candidate
      lo = mid + 1; // look for a closer one above
    } else {
      hi = mid - 1; // too large, look lower
    }
  }

  if (best < 0) {
    /* no symbol whose addr ≤ given addr */
    return "NULL";
  }

  /* 4) Return the name of the best match */
  return (char *)syms->list[best].name;
}

#define MAX_STACK_DEPTH 32
void print_stacktrace(void) {
  uint64 *fp;
  int depth = 0;

  /* read the frame-pointer register into a C pointer */
  asm volatile("mv %0, s0" : "=r"(fp));
  printf("Call Trace:\n");

  /* walk the frame-pointer linked list */
  while (fp && depth < MAX_STACK_DEPTH) {
    uint64 saved_fp = fp[-2];
    uint64 return_addr = fp[-1];
    /* stop if we’ve wrapped or reached user-land or invalid addr */
    if (saved_fp <= (uint64)fp || !lookup_symbol(return_addr))
      break;

    /* look up & print symbol */
    const char *sym = lookup_symbol(return_addr);
    printf("  [%d] 0x%llx: %s\n", depth, (unsigned long long)return_addr,
           sym ? sym : "?");

    fp = (uint64 *)saved_fp;
    depth++;
  }
}

void panic(const char *fmt, ...) {
  push_off();
  panicked = cpuid();

  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
  printf("================================ KERNEL PANIC "
         "================================\n");
  printf("kernel panic on CPU %d\n", panicked);
  printf("caused by:\n\t");
  vprintf(fmt, ap);
  printf("\n");
  printf("kernel_hartid=0x%llx\n", cpuid());
  printf("kernel_pagetable=0x%llx\n", r_satp());
  printf("kernel stack pointer=0x%llx\n", r_sp());
  printf("kernel trapframe=0x%llx\n", (uint64)myproc()->trapframe);
  printf("kernel process pid=%d\n", myproc()->pid);
  printf("kernel process name=%s\n", myproc()->name);
  printf("kernel process state=%d\n", myproc()->state);
  printf("kernel process kstack=0x%llx\n", myproc()->kstack);
  printf("kernel process pagetable=0x%llx\n", myproc()->pagetable);
  printf("kernel process sz=0x%llx\n", myproc()->sz);

  print_stacktrace();

  for (;;) {
    ;
  }
}