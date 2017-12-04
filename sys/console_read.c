#include <sys/defs.h>
#include <sys/memory.h>
#include <sys/kutils.h>
#include <sys/kprintf.h>
#include <sys/console.h>

/*TODO: Decide this size */
#define CONSOLE_BUF_SIZE	1000
char console_read_buf[CONSOLE_BUF_SIZE];

uint64_t cursor = 0;
volatile uint64_t reading_flag = 0;
static volatile int scanlen = 0;

#define ENTER '\n'
#define BKSPACE '\b'

void update_read_buf(char key)
{
	if (key == ENTER) {
		reading_flag = 0;
		console_read_buf[cursor] = '\0';
		cursor = 0;
		return;
	} else if (key == BKSPACE) {
		if (cursor > 0) {
			putchar(key);
			console_read_buf[--cursor] = '\0';
		}
		return;
	} else {
		console_read_buf[cursor++] = key;
		console_read_buf[cursor] = '\0';
		if (reading_flag) {
			putchar(key);
		}
	}
}

/* TODO: Count and fd are not used for now. */
uint64_t console_read(int fd, char *buf, uint64_t count)
{
	scanlen = 0;
	reading_flag = 1;
	__asm__ volatile ("sti");
	while (reading_flag == 1);
	buf[cursor] = '\0';

	__asm__ volatile ("cli");
	/* TODO: memcpy does not seem to work here.  */
	strcpy(buf, console_read_buf);
#if 0
	kprintf("BUF: %s\n", buf);
#endif
	return strlen(buf);
}
