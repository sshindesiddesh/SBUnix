#include <stdio.h>
#include <unistd.h>


int main(int argc, char *argv[], char *env[])
{
	printf("INIT\n");
	char * const argv_my[] = {"/rootfs/bin/echo", "===========================Welcome===========================\n", NULL};
	char * const env_my[] = {"PATH=/rootfs/bin", NULL};
	pid_t pid = fork();
	if (pid == 0) {
		execvpe("/rootfs/bin/echo", argv_my, env_my);
		exit(0);
	}

	waitpid(pid, 0);

	pid = fork();
	if (pid == 0) {
		execvpe("/rootfs/bin/sbush", NULL, env_my);
	}
	waitpid(pid, 0);
	puts("\nSBUSH was killed.. Shutting down system...\n");
	shutdown();
		while (1);
	return 0;
}
