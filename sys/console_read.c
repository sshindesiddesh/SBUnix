#include <sys/defs.h>
#include <sys/memory.h>
#include <sys/kutils.h>
#include <sys/kprintf.h>

/*TODO: Decide this size */
#define CONSOLE_BUF_SIZE	1000
char console_read_buf[CONSOLE_BUF_SIZE];

uint64_t cursor = 0;
volatile uint64_t reading_flag = 0;
static volatile int scanlen = 0;

#define ENTER '\n'

void update_read_buf(char key)
{
	if (key == ENTER) {
		reading_flag = 0;
		return;
	}
	console_read_buf[cursor] = key;
	if (reading_flag) {
		cursor++;
	}
}

/* TODO: Count and fd are not used for now. */
uint64_t console_read(int fd, char *buf, uint64_t count)
{
	cursor = 0;
	scanlen = 0;
	reading_flag = 1;
	__asm__ volatile ("sti");
	while (reading_flag == 1);
	buf[cursor] = '\0';

	/* TODO: memcpy does not seem to work here.  */
	strcpy(buf, console_read_buf);
	cursor = CONSOLE_BUF_SIZE - 1;

	/* clear console_read_buf */
	while(console_read_buf[scanlen])
		scanlen++;

	while(scanlen >= 0) {
		console_read_buf[scanlen] = '\0';
		scanlen--;
	}

	__asm__ volatile ("cli");
	return cursor;
}
