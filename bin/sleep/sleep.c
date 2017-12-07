#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[], char *env[])
{
	if (argc < 2) {
		puts("\nsleep: missing operand");
	} else {
		sleep(stoi(argv[1]));
	}
	return 0;
}
