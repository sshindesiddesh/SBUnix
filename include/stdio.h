#ifndef _STDIO_H
#define _STDIO_H

#include <sys/defs.h>

#define	stdout	1
#define	stdin	0
#define WNOHANG	1

#define	O_RDONLY	0000		/* open for reading only */
#define	O_WRONLY	0001		/* open for writing only */
#define	O_RDWR		0002		/* open for reading and writing */
#define	O_ACCMODE	0003		/* mask for above modes */
#define O_APPEND	2000
#define	O_CREAT		0100
#define EXIT_SUCCESS	0

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define	STDERR_FILENO	2

static const int EOF = -1;

int putc(int c, int fd);
int putchar(int c);
int puts(const char *s);
int printf(const char *format, ...);

char *gets(char *s);

#endif
