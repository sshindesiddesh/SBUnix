#include <sys/kprintf.h>
#include <sys/pic.h>

void __isr_syscall()
{
	kprintf("Syscall\n");
}
