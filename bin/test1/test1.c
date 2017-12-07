#include <stdio.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
	int a = 0;
	pid_t pid = fork();
	if (pid == 0) {
		while (1) {
			a++;
			if (a == 1000000) {
				printf("Hi %d\n", getpid());
				a = 0;
			}
		}
	} else {
		while (1) {
			a++;
			if (a == 1000000) {
				printf("Bye %d\n", getpid());
				a = 0;
			}
		}
	}
        return 0;
}
