#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[], char *env[])
{
	char * const argv_my[] = {"/rootfs/bin/sbush", "/rootfs/etc/rc", NULL};
	char * const env_my[] = {"PATH=/rootfs/bin", NULL};
	pid_t pid = fork();
	if (pid == 0) {
		execvpe("/rootfs/bin/sbush", argv_my, env_my);
		exit(0);
	}

	waitpid(pid, 0);

	pid = fork();
	if (pid == 0) {
		execvpe("/rootfs/bin/sbush", NULL, env_my);
	}

	while ((int)wait(0) >= 0);

	puts("\nSBUSH was killed.. Shutting down system...\n");
	shutdown();
	return 0;
}
