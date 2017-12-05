#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

//char *getenv(const char *name);
//char* strcpy(char *dst, const char *src);

int main(int argc, char *argv[])
{
	printf("in echo %d\n", argc);
	if (argc < 2) {
		puts("\n");
	} else {
		printf("%s\n", argv[1]);
		if(*argv[1] == '$') {
			printf("Here\n");
			char *buf = getenv("PATH");
			printf("%p\n", buf);
			/* echo env var if available */
			//char buf[100] = "\0";
			//puts(getenv(argv[1] + 1));
			//strcpy(buf, getenv(&(*argv[1] + 1)));
			//puts(((char *)&(argv[1][1])));
			//puts(getenv((char *)&(argv[1][1])));
			//printf("%s\n", getenv("PATH"));
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
