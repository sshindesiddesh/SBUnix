#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int putc(int c, int fd)
{
	return write((long)fd, &c, 1);
}
