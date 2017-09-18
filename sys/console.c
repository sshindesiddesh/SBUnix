#include <sys/kprintf.h>
#include <sys/defs.h>
#include <stdarg.h>
#include <sys/console.h>

unsigned int cur = 0;
unsigned int write_cnt = 0;

unsigned short *textptr = (unsigned short *)0xB8000;;
char screen_buf[MAX_SCREEN_SIZE] = {' '};
char write_buf[MAX_WRITE_SIZE] = {' '};
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
	char *p = (out + 99);
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

/* Update the data in write_buf on the screen */
/* TODO: Currently the screen scrolling is well supported only if all the
 * print statements end on a line. i.e. ending with \n
 */
void update()
{
	#if 0	
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
	#endif
	int temp = 0;
	unsigned blank_c = 0x20;
	if(y_pos >= 24) {
		temp = y_pos - 24 + 1;
		memcpy(textptr, textptr + temp * 80, (24 - temp) * 80 * 2);
		memsetw (textptr + (24 - temp) * 80, blank_c, 80);
		y_pos = 23;
	}
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
	unsigned short *c_temp;
	/* handle positions of x and y coordinates of the screen as per the character getting put */
	if (c == 0x08) {	/* back space */
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
	else if(c >= 0x20) {	/* for all the characters >= space (0x20)*/
		c_temp = textptr + (y_pos * 80 + x_pos);
		*c_temp = c;
		x_pos++;
	}
	if (x_pos >= 80) {	/* Shift the x coordinate to new line after 80 units */
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
	unsigned blank_c = 0x20;
	for(int i = 0; i < 25; i++)
		memsetw((textptr + i * 80), (blank_c), 80);
	x_pos = 0;
    	y_pos = 0;
	return 0;
}
