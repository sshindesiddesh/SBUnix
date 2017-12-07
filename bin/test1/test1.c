#include <stdio.h>
#include <unistd.h>
int b = 0;
int main(int argc, char *argv[])
{
	int a = 0;
	pid_t pid = fork();
	if (pid == 0) {
		while (1) {
			a++;
			if (a == 100)
				a = 0;
			b++;
			if (b == 100)
				b = 0;
		}
	} else {
		while (1) {
			a++;
			if (a == 100)
				a = 0;
			b++;
			if (b == 100)
				b = 0;
		}
	}
        return 0;
}
