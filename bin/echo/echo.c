#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[], char *env[])
{
	int i = 0;
	char *ptr;
#if 0
	printf("In echo\n");

	if (argv) {
		while (argv[i]) {
			printf("%s\n", argv[i++]);
		}
	}
	i  = 0;
	if (env) {
		while (env[i]) {
			printf("%s\n", env[i++]);
		}
	}
#endif
	puts("\n");
	if (argc >= 2) {
		if(*argv[1] == '$') {
			if (env) {
				while (env[i]) {
					ptr = ((char *)&(argv[1][1]));
					if (!strncmp(env[i], ptr, strlen(ptr))) {
						ptr = ((char *)&(env[i][strlen(ptr)+1]));
						puts(ptr);
						return 0;
					}
					i++;
				}
			}
		} else {
			int i = 1;
			while(argc > 1) {
				puts(argv[i]);
				puts(" ");
				i++;
				argc--;
			}
		}
	}

	return 0;
}
