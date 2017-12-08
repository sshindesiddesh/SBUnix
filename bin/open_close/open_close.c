#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/* binary to test all basic functionality at a single point */
char buffer[100] = "\0";

int main(int argc, char *argv[], char *env[])
{
	/* test for open, read, close in a file, assuming /rootfs/bin/cat already present */
	int f, x = 1, c = 0;
	f = open("/rootfs/etc/help", O_RDONLY);

	if (f < 0) {
		write(1, "\nCould not open the file", 30);
	} else {
		printf("\ncontent of cat binary:\n");
		while (x != 0) {
			x = read(f, &c, 1);
			write(1, &c, 1);
		}
		close(f);
	}

	/* Opendir Readdir Closedir */
	DIR * dir = opendir("/rootfs/etc1/");
	if(dir == NULL) {
		puts("\nDirectory does not exist:");
		puts("/rootfs/etc1/");
	} else {
		puts("\nDirectory exists");
	}

	dir = opendir("/rootfs/bin/");
	if(dir == NULL) {
		puts("\nDirectory does not exist");
	} else {
		puts("\nDirectory exists:");
		puts("/rootfs/bin/");

		struct dirent* cur_dir = NULL;
		while((cur_dir = readdir(dir)) != NULL) {
			puts("    ");
			puts(cur_dir->d_name);
		}
		closedir(dir);

	}

	return 0;
}
