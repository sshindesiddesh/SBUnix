#include <sys/timer.h>
#include <sys/kprintf.h>

int PIT_RELOAD_VAL = 5965;

static void outb(uint16_t port, uint8_t val)
{
	__asm__ __volatile__ ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

void timer_init () 
{

		// Send the command byte. 
		outb (0x43, 0x36); //channel 0, lobyte/hibyte, mode 2:110b 

		// Divisor has to be sent byte-wise 
		uint8_t lobyte = (uint8_t) (PIT_RELOAD_VAL & 0xFF); 
		uint8_t hibyte = (uint8_t) ((PIT_RELOAD_VAL >> 8) & 0xFF); 

		// Send the frequency divisor. 
		outb (0x40, lobyte); 
		outb (0x40, hibyte); 
}


void __isr_timer_cb(uint64_t count)
{
	kprintf("\nAhdvdasvfakvdf\n");
	return;
}
