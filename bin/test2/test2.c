#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[])
{
	int a = 0;
	while (a < 990) {
	pid_t pid = fork();
		if (pid == 0) {
			sleep(50);
			exit(0);
		}
		a++;
	}
	puts("Parrent Spawned...\n");
        return 0;
}
