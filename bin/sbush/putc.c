#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int putc(int c, int fd)
{
	write((long)fd, &c, 1);
	return c;
}
