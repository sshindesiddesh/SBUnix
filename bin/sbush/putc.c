#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int put_c(int c, int fd)
{
	return write((long)fd, &c, 1);
}
