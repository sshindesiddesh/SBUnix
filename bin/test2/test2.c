#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[])
{
	int a = 0;
	pid_t pid;

	while (a < 100) {
	pid = fork();
	if (pid == 0) {
		execvpe("/rootfs/bin/test10", NULL, NULL);
	}
		a++;
	}
	puts("Parrent Spawned...\n");
        return 0;
}
