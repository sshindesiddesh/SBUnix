#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

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

size_t get_line(int fp, char *buf)
{
	int ret = -1;
	if ((ret = read(fp, buf, 1024)) == -1) {
		exit(0);
	}
	buf[ret - 1] = '\0';
	return ret;

}

int main(int argc, char *argv[], char *env[])
{
	write(1, "\n", 2);
	int x = 1, c = 0;
	if(argc > 1)
	{
		/* first check if its a directory */
		DIR *status = opendir(argv[1]);
		if (status) {
			write(1, "Input is a directory\n", 30);
		} else {
			int f = open(argv[1], O_RDONLY);
			if (f < 0) {
				write(1, "Could not open the file\n", 30);
				return 0;
			}
			while (x != 0) {
				x = read(f, &c, 1);
				write(1, &c, 1);
			}
		}
	} else {
		write(1, "Usage: cat <file_name>\n", 30);
	}

#if 0
	int x = 1, c = 0;
	if (argc == 2) {
		int f = open(argv[1], O_RDONLY, 444);
		if (f < 0) {
			print("Error in input file\n");
			return 0;
		}
		while (x != 0) {
			x = read(f, &c, 1);
			write(1, &c, x);
		}
	}
#endif
        return 0;
}
