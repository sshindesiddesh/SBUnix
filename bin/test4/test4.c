#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[])
{
	int a = 0, b = 0;
	while (a < 20) {
	pid_t pid = fork();
		if (pid == 0) {
			while (1) {
				b++;
				if (b == 100000) {
					b = 0;
					write(1, "Child\n", 7);
				}
			}
		}
		a++;
	}
	puts("Parrent Spawned...\n");
        return 0;
}
