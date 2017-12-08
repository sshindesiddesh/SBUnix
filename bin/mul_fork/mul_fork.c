#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[])
{
	int a = 0;
	pid_t pid;
	char *argv1[] = {"/rootfs/bin/echo", "Hello\n", NULL};

	while (a < 20) {
		pid = fork();
		if (pid == 0) {
			execvpe("/rootfs/bin/echo", argv1, NULL);
		}
		a++;
	}
	puts("Parrent Spawned...\n");
        return 0;
}
