#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

void printdir(char *path)
{
	DIR *dir = opendir(path);
	if (dir < 0) {
		write(1, "Invalid path", 20);
		return;
	}
	else {
		write(1, "\n", 2);
		struct dirent *d;
		while ((d = (struct dirent *)readdir(dir)) != NULL) {
			write(1, (d->d_name), strlen(d->d_name));
			write(1, "\t", 5);
		}
	}
	closedir(dir);
}

int main(int argc, char *argv[], char *envp[])
{
	char cwd[50] = "\0";
	if (argc < 2) {
		getcwd(cwd, sizeof(cwd));
		printdir(cwd);
		return 0;
	}

	/* check if the input is a directory or not */
	DIR *status = opendir(argv[1]);
	if (status) {
		printdir(argv[1]);
	} else {
		write(1, "\n Input is not a directory", 30);
	}

	return 0;
}
