#include <sys/timer.h>
#include <sys/kprintf.h>
#include <sys/pic.h>
#include <sys/config.h>
#include <sys/process.h>

void decrement_sleep_count();
void update_time(uint64_t time);
extern uint64_t reading_flag;
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

extern int sleep_cnt;;
void __isr_timer_cb(uint64_t count)
{
	k++;
	if (k == 18) {
		/* kprintf("Cnt %d\n", i); */
		update_time(i++);
		if(sleep_cnt > 1)
			decrement_sleep_count();
		k = 0;
	}
	/* Acknowledge PIC */
	pic_ack(0x20);
}

void kyield();

void pre_empt_yield(void)
{
	/* pre-empt 1/2 a second */
	/* if ((k%9 == 0) && reading_flag == 0) {
	 * SBUSH waits on read where reading flag is set for most of the times */
	if ((k%3 == 0)) {
		kyield();
	}
}
