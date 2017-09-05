#ifndef _STDIO_H
#define _STDIO_H

#include <sys/defs.h>
#define	stdout	1
#define	stdin	0

#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

static const int EOF = -1;

int getc(int fd);
int putc(int c, int fd);
int puts(const char *s);
int printf(const char *format, ...);

char *gets(char *s);

#endif
