#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[], char *env[])
{
	if (argc < 3) {
		puts("\nusage: kill -9 <pid>");
	} else {
		if(!strcmp(argv[1], "-9")) {
			if (stoi(argv[2]) != 0)
				kill(stoi(argv[2]));
		} else {
			puts("\nusage: kill -9 <pid>");
		}
	}
	return 0;
}
