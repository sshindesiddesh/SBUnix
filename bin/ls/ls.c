#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

void print(char *s)
{
	while (*s != '\0')
		putc(*s++, stdout);
}

void printdir(char *path)
{
	DIR *dir = opendir(path);
	if (!dir) {
		print("Invalid Path");
		return;
	}

	struct dirent *ep;
	while (ep = readdir(dir)) {
		print(ep->d_name);
		putc('\t', stdout);
	}

}

int main(int argc, char *argv[])
{
	char buf[256];
	char *dir = buf;


	if (argc == 1) {
		dir = getcwd(dir, sizeof(buf));
		printdir(dir);
	} else if (argc == 2) {
		printdir(argv[1]);
	}
	putc('\n', stdout);
	return 0;
}
