#include <stdio.h>
#include <sys/defs.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[], char *envp[])
{
	int i = 0;
	char *a, *b, *c;
	while(i < 500) {
		a = (char *)malloc(900000);
		strcpy(a+80000, "h");
		b = (char *)malloc(1000);
		free(a);
		free(b);
		a = (char *)malloc(1000);
		c = (char *)malloc(200);
		strcpy(c, "hello there c");
		free(c);
		free(a);
		i++;
	}
	write(1, "\nmalloc tested", 20);
	return 0;
}
