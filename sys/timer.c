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
	__asm__ __volatile__(
        "pushq %rdi\n"
        "pushq %rax\n"
        "pushq %rbx\n"
        "pushq %rcx\n"
        "pushq %rdx\n"
        "pushq %rbp\n"
        "pushq %rsi\n"
        "pushq %r8\n"
        "pushq %r9\n"
        "pushq %r10\n"
        "pushq %r11\n"
        "pushq %r12\n"
        "pushq %r13\n"
        "pushq %r14\n"
        "pushq %r15\n"
        );
        
	__asm__ __volatile__(
        "popq %r15\n"
        "popq %r14\n"
        "popq %r13\n"
        "popq %r12\n"
        "popq %r11\n"
        "popq %r10\n"
        "popq %r9\n"
        "popq %r8\n"
        "popq %rsi\n"
        "popq %rbp\n"
        "popq %rdx\n"
        "popq %rcx\n"
        "popq %rbx\n"
        "popq %rax\n"
	"popq %rdi\n"
	);
	
	k++;
	if (k == 18) {
		/* kprintf("Cnt %d\n", i); */
		update_time(i++);
		k = 0;
	}
	/* Acknowledge PIC */
	pic_ack(0x20);
}
