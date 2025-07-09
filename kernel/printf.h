#ifndef PRINTF_H
#define PRINTF_H
int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void panic(const char *why) __attribute__((noreturn));
void putstr(const char *s);
void printfinit(void);
#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)
#define kassert(x, why)                                                        \
  do {                                                                         \
    if (!(x)) {                                                                \
      panic("Assertion failed: " #x " in " __FILE__ " at line " LINE_STRING    \
            ": " why);                                                         \
    }                                                                          \
  } while (0)
#endif // PRINTF_H