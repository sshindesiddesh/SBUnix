#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[])
{
	int i = 0;
	for (i  = 0; i < 1000000; i++) {}
	printf("In Test %d\n", getpid());
	for (i  = 0; i < 1000000; i++) {}
        return 0;
}
