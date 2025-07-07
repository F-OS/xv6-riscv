#ifndef PRINTF_H
#define PRINTF_H
int printf(char *fmt, ...) __attribute__((format(printf, 1, 2)));
void panic(char *why) __attribute__((noreturn));
void printfinit(void);
#endif // PRINTF_H