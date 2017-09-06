#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int x = 0;
	if (argc == 2) {
		FILE f = open(argv[1], O_RDWR);
		if (!f)
			return 0;
		while (x != EOF) {
			x = getc(f);
			putc(x, stdout);
		}
	}
        return 0;
}
