#include <sys/kprintf.h>
#include <sys/defs.h>
#include <stdarg.h>

/* -80 to skip the last line which is reserved for Timer. */
#define MAX_SCREEN_SIZE	2000 - 80
#define MAX_WRITE_SIZE	200


static unsigned int cur = 0;
static unsigned int write_cnt = 0;

char screen_buf[MAX_SCREEN_SIZE] = {' '};
char write_buf[MAX_WRITE_SIZE] = {' '};

char *generic_conv(long n, int b);

/* Returns Length of the String */
size_t strlen(const char *buf)
{
	size_t len = 0;
	while (*buf++ != '\0') len++;
	return len;
}

/* Memory Copy function */
void *memcpy(void *dest, const void *src, int n)
{
	char *d = (char *)dest;
	char *s = (char *)src;
	int i;
	for (i = 0; i < n; i++)
		d[i] = s[i];
	return d;
}

void update_time(uint64_t time)
{
		char *str = generic_conv(time, 10);
		register char *temp1, *temp2;
		int i = 0;

		char *str1 = "Seconds since Boot ";

		for (temp1 = str1, temp2 = (char*)0xb8000 + 3840; i < strlen(str1); temp1 += 1, temp2 += 2, i++)
				*temp2 = *temp1;
		i = 0;
		for (temp1 = str, temp2 = (char*)0xb8000 + 3840 + 40; i < strlen(str); temp1 += 1, temp2 += 2, i++)
				*temp2 = *temp1;
}

void update_key(int key, int ctrl)
{
		register char *temp1, *temp2;
		int i = 0;
		char *str1 = "Last Pressed Key ";

		for (temp1 = str1, temp2 = (char*)0xb8000 + 3900; i < strlen(str1); temp1 += 1, temp2 += 2, i++)
				*temp2 = *temp1;

		temp2 = (char*)0xb8000 + 3936;

		if (ctrl == 1) {
			*temp2 = '^';
			temp2 += 2;
		}
		*temp2 = (char)key;
		temp2 += 2;
		*temp2 = ' ';
}

/* Update the data in write_buf on the screen */
/* TODO: Currently the screen scrolling is well supported only if all the
 * print statements end on a line. i.e. ending with \n
 */
void update()
{
	char *b = screen_buf;
	/*  If the cursor has reached the end */
	if (cur >= MAX_SCREEN_SIZE - 1) {
		/* Move the screen buffer to fit the new write buffer */
		memcpy(b, b + write_cnt, MAX_SCREEN_SIZE - write_cnt);
		/* Copy the Write buffer to the end of the screen buffer */
		memcpy(b + MAX_SCREEN_SIZE - write_cnt, write_buf, write_cnt);
	} else {
		/* Copy the Write buffer to the end of the screen buffer */
		memcpy(b + cur, write_buf, write_cnt);
		cur += write_cnt;
	}
	/* The cursor has stil not reached the end */
	register char *temp1, *temp2;
	int i = 0;

	for (temp1 = b, temp2 = (char*)0xb8000; i < cur; temp1 += 1, temp2 += 2, i++)
		*temp2 = *temp1;
	/* Print spaces if cursor is at something less than 2000. */
	for (temp2 = (char*)0xb8000 + 2 * cur; i < MAX_SCREEN_SIZE; temp2 += 2, i++)
		*temp2 = ' ';
	/* Make write_cnt 0 for next iteration */
	write_cnt = 0;
}

/* Writes to a write buffer which is later copied to the screen buffer in update. */
void write(void *b_in, int n)
{
	char *out = write_buf;
	char *in = (char *)b_in;

	int i = 0;
	while (i < n) {
		out[write_cnt + i] = in[i];
		i++;
		write_cnt++;
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

/* Conversion Function for all required base */
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
