#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

size_t strlen(const char *buf);
char *strcat(char *dst, const char* src);
char* strcpy(char *dst, const char *src);

int put_c(int c, int fd)
{
	return write((long)fd, &c, 1);
}

void print(char *s)
{
	while (*s != '\0')
		put_c(*s++, 1);
}

char buf[1024];

#if 0
void printdir(char *path)
{
	int dir = open(path, O_RDONLY, 444);
	if (dir < 0) {
		print("Invalid Path");
		return;
	}

	char *b = buf;
	int i = 0, read = 1, k = 0;
	struct dirent *d;
	while (read > 0) {
		read = getdents(dir, b, sizeof(buf));
		while (i < read) {
			d = (struct dirent *)(buf + i);
			print(d->d_name);
			print("   ");
			if ((k % 5 == 0) && k != 0)
				print("\n");
			i += d->d_reclen; k++;
		}
	}

	close(dir);
}
#endif

void printdir1(char *path)
{
	write(1, "\n", 2);
	int dir = open(path, O_RDONLY);

	if (dir < 0) {
		write(1, "Invalid path", 20);
		return;
	}
	else {
		char *b = buf;
		struct dirent *d;
		d = (struct dirent *)getdents(dir, b, sizeof(buf));
		while ((d != NULL) && (strlen(d->d_name) > 0)) {
			write(1, (d->d_name), strlen(d->d_name));
			write(1, "\t", 5);
			d = (struct dirent *)getdents(dir, b, sizeof(buf));
		}
	}
	close(dir);
}

int main(int argc, char *argv[], char *env[])
{
	char cwd[50] = "\0";
	if (argc < 2) {
		getcwd(cwd, sizeof(cwd));
		printdir1(cwd);
		return 0;
	}

	/* check if the input is a directory or not */
	DIR *status = opendir(argv[1]);
	if (status) {
		printdir1(argv[1]);
	} else {
		write(1, "\n Input is not a directory", 30);
	}

	return 0;
}
