#include <sys/kprintf.h>
#include <sys/defs.h>
#include <stdarg.h>
#include <sys/console.h>
#include <sys/kutils.h>

unsigned short *textptr = (unsigned short *)0xB8000;
unsigned short *clrptr = (unsigned short *)0xB8001;

int x_pos = 0, y_pos = 0;

void change_console_ptr()
{
	//clear();
	textptr = (unsigned short *)0xFFFFFFFF800B8000;
	clrptr = (unsigned short *)0xFFFFFFFF800B8001;
}

/* Write character at specific position */
int write_console(int c, int x, int y)
{
	unsigned short *c_temp, *clr;
	c_temp = textptr + (y * MAX_SCREEN_X + x);
	clr = clrptr + (y * MAX_SCREEN_X + x);
	*c_temp = c;
	*clr = 7;
	return c;
}

/* Conversion Function for all required base */
char *generic_conv(unsigned long n, int b)
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
	const char *str1 = "Time since Boot: ";
	char *str = generic_conv(time, 10);
	int i, l = strlen(str1);
	for (i = 0; i < l; i++) {
		write_console(str1[i], 54 + i, 24);
	}
	l = strlen(str);
	for (i = 0; i < l; i++) {
		write_console(str[i], 72 + i, 24);
	}
}

void update_key(int key, int ctrl)
{
		if (ctrl == 1) {
			write_console('^', 50, 24);
			write_console(key, 51, 24);
		} else {
			write_console(key, 50, 24);
			write_console(' ', 51, 24);
		}
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
	/* handle positions of x and y coordinates of the screen as per the character getting put */
	if (c == '\b') {	/* back space */
		if (x_pos != 0) {
			write_console(' ', --x_pos, y_pos);
		}
	}
	else if(c == '\r') {	/* Carriage return */
		x_pos = 0;
	}	
	else if(c == '\n') {	/* new line */
		x_pos = 0;
		y_pos++;
	}
	else if(c == '\t') {	/* tab */
		x_pos = x_pos + 8;
	}	
	else if(c >= ' ') {	/* for all the characters >= space (0x20)*/
		write_console(c, x_pos, y_pos);
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
