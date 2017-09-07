#include <stdio.h>
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

size_t get_line(int fp, char *buf)
{
	int ret = -1;
	if ((ret = read(fp, buf, 1024)) == -1) {
		exit(0);
	}
	buf[ret - 1] = '\0';
	return ret;

}

int main(int argc, char *argv[])
{
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
        return 0;
}
