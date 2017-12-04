#ifndef _KUTILS_H
#define _KUTILS_H
#include <sys/defs.h>
char *mystrtok_r(char *bstr, const char *delim, char **save);
char *strtok(char *bstr, const char *delim);
size_t strncmp(const char *s1, const char *s2, int len);
char* strcpy(char *dst, const char *src);
size_t strlen(const char *buf);
size_t strcmp(const char *s1, const char *s2);
long stoi(const char *s);
uint64_t power(uint64_t num, int e);
void *memcpy(void *dest, const void *src, int n);
void *memset(void *ptr, int value, size_t len);
unsigned short *memsetw(unsigned short *dest, unsigned short src, size_t count);
uint64_t octal_to_decimal(uint64_t octal);
char *strcat(char *dst, char* src);
void zero_out(void *buf, int b);

#endif
