#include <sys/kprintf.h>
#include <sys/defs.h>
#include <stdarg.h>
#include <sys/console.h>

extern unsigned int cur;
extern unsigned int write_cnt;

void kprintf(const char *fmt, ...)
{
	char *t, *str;
	va_list arg;
	unsigned int i;

	va_start(arg, fmt);

	/* Collect all the data in a write buffer */
	for (t = (char *)fmt; *t != '\0'; t++) {
		while ((*t != '%') && (*t != '\0')) {
			if (*t == '\n') {
				int space_rem = (80 - (cur + write_cnt)%80);
				/* Fill the write buffer with spaces for the rest of the line */
				for (i = 0; i < space_rem; i++)
					putchar(' ');
				t++;
				continue;
			}
			putchar(*t);
			t++;
		}
		if (*t == '\0')
			break;
		t++;
		switch (*t) {
			case 'd':
				i = va_arg(arg, int);
				if (i < 0) {
					putchar('-');
					str = generic_conv(-i, 10);
				} else
					str = generic_conv(i, 10);
				puts(str);
				break;
			case 'p' :
				i = va_arg(arg, unsigned int);
				puts("0x");
				puts(generic_conv(i, 16));
				break;
			case 'x':
				i = va_arg(arg, unsigned int);
				puts(generic_conv(i, 16));
				break;
			case 'c' :
				i = va_arg(arg, int);
				putchar(i);
				break;
			default:
				break;
		}

	}

	/* Update the data on screen */
	update();
}
