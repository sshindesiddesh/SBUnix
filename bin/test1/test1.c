#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[], char *envp[])
{
	int a = 0;
	pid_t pid = fork();
	if (pid == 0) {
		while (1) {
			a++;
			if (a == 1000000) {
				a = 0;
			}
		}
	} else {
		while (1) {
			a++;
			if (a == 1000000) {
				a = 0;
			}
		}
	}
        return 0;
}
