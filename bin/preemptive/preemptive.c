#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[], char *envp[])
{
	int a = 100;
	pid_t pid = fork();
	printf("Problem");
	if (pid == 0) {
		while (1) {
			a++;
			if (a == 1000000) {
				a = 0;
				write(1, "Parent\n", 7);
			}
		}
	} else {
		while (1) {
			a++;
			if (a == 1000000) {
				a = 0;
				write(1, "Child\n", 6);
			}
		}
	}
        return 0;
}
