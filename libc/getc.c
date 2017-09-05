#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int getc(int fd)
{
	int c = 0, ret = -1;

	while (ret < 1) {
		ret = read((long)fd, &c, 1);
	}

	if (ret == 1)
		return c;
	else
		return -1;
}
