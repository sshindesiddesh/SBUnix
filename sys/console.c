#include <sys/kprintf.h>
#include <sys/defs.h>
#include <stdarg.h>
#include <sys/console.h>

unsigned short *textptr = (unsigned short *)0xB8000;

int x_pos = 0, y_pos = 0;

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

/* Memcopy function for 16 bit values */
unsigned short *memsetw(unsigned short *dest, unsigned short src, size_t count)
{
    unsigned short *t = (unsigned short *)dest;
    for( ; count != 0; count--) *t++ = src;
    return dest;
}

/* Conversion Function for all required base */
char *generic_conv(long n, int b)
{
	char fixed[] = "0123456789ABCDEF";
	static char out[100];
	char *p = (out + sizeof(out) - 1);
	*p = '\0';
	do {
		*--p = fixed[n%b];
		n /= b;
	} while (n);
	return p;
}

void update_time(uint64_t time)
{
		char *str = generic_conv(time, 10);
		register char *temp1, *temp2;
		int i = 0;
#if 0

		char *str1 = "Seconds since Boot ";

		for (temp1 = str1, temp2 = (char*)0xb8000 + 3840; i < strlen(str1); temp1 += 1, temp2 += 2, i++)
				*temp2 = *temp1;
#endif
		i = 0;
		for (temp1 = str, temp2 = (char*)0xb8000 + 3980; i < strlen(str); temp1 += 1, temp2 += 2, i++)
				*temp2 = *temp1;
}

void update_key(int key, int ctrl)
{
		register char *temp1, *temp2;
#if 0
		int i = 0;
		char *str1 = "Last Pressed Key ";

		for (temp1 = str1, temp2 = (char*)0xb8000 + 3900; i < strlen(str1); temp1 += 1, temp2 += 2, i++)
				*temp2 = *temp1;
#endif
		temp1 = NULL;
		temp1++;

		temp2 = (char*)0xb8000 + 3970;

		if (ctrl == 1) {
			*temp2 = '^';
			temp2 += 2;
		}
		*temp2 = (char)key;
		temp2 += 2;
		*temp2 = ' ';
}

/* Screen scrolling supported here */
void update()
{
	int temp = 0;
	unsigned blank_c = ' ';
	if(y_pos >= MAX_SCREEN_Y) {
		temp = y_pos - MAX_SCREEN_Y + 1;
		memcpy(textptr, textptr + temp * MAX_SCREEN_X, (MAX_SCREEN_Y - temp) * MAX_SCREEN_X * 2);
		memsetw(textptr + (MAX_SCREEN_Y - temp) * MAX_SCREEN_X, blank_c, MAX_SCREEN_X);
		y_pos = MAX_SCREEN_Y - 1;
	}
}

int putchar(int c)
{
	unsigned short *c_temp;
	/* handle positions of x and y coordinates of the screen as per the character getting put */
	if (c == '\b') {	/* back space */
		if (x_pos != 0)
			x_pos--;
	}
	else if(c == '\r') {	/* Carriage return */
		x_pos = 0;
	}	
	else if(c == '\n') {	/* new line */
		x_pos = 0;
		y_pos++;
	}
	else if(c >= ' ') {	/* for all the characters >= space (0x20)*/
		c_temp = textptr + (y_pos * MAX_SCREEN_X + x_pos);
		*c_temp = c;
		x_pos++;
	}
	if (x_pos >= MAX_SCREEN_X) {	/* Shift the x coordinate to new line after 80 units */
		x_pos = 0;
		y_pos++;
	}
	update();
	return c;
}

int puts(const char *str)
{
	int i = 0;
	while (str[i] != '\0')
		putchar(str[i++]);
	return i;
}

int clear()
{
	unsigned blank_c = ' ';
	for(int i = 0; i < MAX_SCREEN_Y; i++)
		memsetw((textptr + i * MAX_SCREEN_X), (blank_c), MAX_SCREEN_X);
	x_pos = 0;
    	y_pos = 0;
	return 0;
}
