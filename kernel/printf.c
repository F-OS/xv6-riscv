//
// formatted console output -- printf, panic.
//

#include <stdarg.h>

#include "console.h"
#include "kernel/param.h"
#include "kernel/proc.h"
#include "kernel/sbi.h"
#include "spinlock.h"
#include "types.h"

volatile bool panicked = false;


static char digits[] = "0123456789abcdef";

// Print to the console.
char printbuf[NCPU][256] = {}; // buffer for printf
int bufpos[NCPU] = {0};    // current position in the buffer

void bufwrite(int c) {
  int cpu_id = cpuid();
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
  int cpu_id = cpuid();
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

int printf(const char *fmt, ...) {
  push_off();
  va_list ap;
  int i = 0;
  int cx = 0;
  int c0 = 0;
  int c1 = 0;
  int c2 = 0;
  const char *s = NULL;


  va_start(ap, fmt);
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
  va_end(ap);

  bufflush();


  pop_off();

  return 0;
}

void putstr(const char *s) {
  if (s == 0) {
    s = "(null)";
  }
  sbi_debug_console_write(s);
}

void panic(const char *s) {
  sbi_debug_console_write("panic: ");
  sbi_debug_console_write(s);
  sbi_debug_console_write("\n");
  panicked = true; // freeze uart output from other CPUs
  for (;;) {
    ;
  }
}