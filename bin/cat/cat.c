#include <stdio.h>

int main(int argc, char *argv[])
{
	int x = 0;
	if (argc == 2) {
		
		FILE *f = fopen(argv[1], "r");
		if (!f)
			return 0;
		while (x != EOF) {
			x = getc(f);
			putc(x, stdout);
		}
	}
        return 0;
}
