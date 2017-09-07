#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

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

void printdir(char *path)
{
	int dir = open(path, O_RDONLY);
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

int main(int argc, char *argv[])
{
	printdir(argv[1] ? argv[1] : ".");
	put_c('\n', stdout);
	return 0;
}
