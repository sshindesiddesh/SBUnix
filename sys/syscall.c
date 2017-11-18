#include <sys/kprintf.h>
#include <sys/syscall.h>

void yield();

uint64_t __isr_syscall(syscall_in *in)
{
	if (in->syscall_no == 1) {
		kprintf("%s", (char *)in->first_param);
	} else if (in->syscall_no == 2) {
		yield();
	} else {
		kprintf("SBUnix x86_64 OS\n");
	}
	return 0;
}
