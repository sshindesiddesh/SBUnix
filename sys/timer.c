#include <sys/timer.h>
#include <sys/kprintf.h>
#include <sys/pic.h>

void timer_init()
{
		/* The system clock is at 1193180.
		 * By default the devisor of 16 bit is 0xFFFF which makes
		 * to trigger at 18.22 hz.
		 * We approximately increment a second after 18 timer invocations
		 */
		/* Send the command byte.*/
		outb(0x43, 0x36);
		outb(0x40, 0xFF);
		outb(0x40, 0xFF);
}

uint64_t k = 0, i = 0;

void update_time(uint64_t time);

void __isr_timer_cb(uint64_t count)
{
	k++;
	if (k == 18) {
		/* kprintf("Cnt %d\n", i); */
		update_time(i++);
		k = 0;
	}
	/* Acknowledge PIC */
	pic_ack(0x20);
}
