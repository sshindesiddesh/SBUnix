#include <sys/kprintf.h>
#include <stdarg.h>

#define MAX_SCREEN_SIZE	2000


static unsigned int cur = 0;

char buf[MAX_SCREEN_SIZE] = {' '};

void *memcpy(void *dest, const void *src, int n)
{
	char *s = (char *)src;
	char *d = (char *)dest;
	int i;
	for (i = 0; i < n; i++)
		d[i] = s[i];
	return d;
}

void update()
{
	register char *temp1, *temp2;
	char *ptr = buf;
	int i = 0;
	for (temp1 = ptr, temp2 = (char*)0xb8000; i < cur; temp1 += 1, temp2 += 2, i++)
		*temp2 = *temp1;
	for (temp1 = ptr, temp2 = (char*)0xb8000 + 2 * cur; i < MAX_SCREEN_SIZE; temp1 += 1, temp2 += 2, i++)
		*temp2 = ' ';
}

void write(void *b_in, int n)
{
	char *out = buf;
	char *in = (char *)b_in;

	int i = 0;
	while (i < n) {
		out[cur + i] = in[i];
		i++;
		cur++;
	}
}

int putchar(int c)
{
	write(&c, 1);
	return c;
}

int puts(const char *str)
{
	int i = 0;
	while (str[i] != '\0')
		putchar(str[i++]);
	return i;
}

char *generic_conv(long n, int b)
{
	char fixed[] = "0123456789ABCDEF";
	static char out[100];
	char *p = (out + 99);
	*p = '\0';
	do {
		*--p = fixed[n%b];
		n /= b;
	} while (n);
	return p;
}

void kprintf(const char *fmt, ...)
{
	char *t, *str;
	va_list arg;
	unsigned int i;

	va_start(arg, fmt);
	for (t = (char *)fmt; *t != '\0'; t++) {
		while ((*t != '%') && (*t != '\0')) {
			if (*t == '\n') {
				cur = cur / 80;
				cur = cur * 80;
				cur += 80;
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
			default:
				break;
		}

	}
	update();
}
