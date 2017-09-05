#ifndef _STDIO_H
#define _STDIO_H

#include <sys/defs.h>
#define	stdout	(void *)1
#define	stdin	(void *)0
static const int EOF = -1;

int getc(void *fd);
int putc(int c, void *fd);
int puts(const char *s);
int printf(const char *format, ...);

char *gets(char *s);

#endif
