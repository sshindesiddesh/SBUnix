#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* EOF flag for file handling */
int __eof;

int getc(int fd)
{
	int c = 0, ret = -1;

	while (ret < 1) {
		ret = read((long)fd, &c, 1);
		if (c == 0 || c == EOF) {
			__eof = 1;
			return -1;
		}
	}

	if (ret == 1)
		return c;
	else
		return -1;
}
