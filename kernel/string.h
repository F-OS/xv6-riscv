#ifndef STRING_H
#define STRING_H

#include "types.h"

void *memset(void *dst, int c, uint n);
int memcmp(const void *v1, const void *v2, uint n);
void *memmove(void *dst, const void *src, uint n);
void *memcpy(void *dst, const void *src, uint n);
void *memchr(const void *ptr, int ch, long unsigned count);
int strncmp(const char *p, const char *q, uint n);
char *strncpy(char *s, const char *t, int n);
int strcmp(const char *p, const char *q);
char *safestrcpy(char *s, const char *t, int n);
char *strcpy(char *s, const char *t);
char *strcat(char *s, const char *t);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
int strlen(const char *s);
long unsigned strnlen(const char *s, long unsigned maxlen);
#endif // STRING_H
