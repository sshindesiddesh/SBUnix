#include <sys/kprintf.h>
#include <sys/syscall.h>
#include <sys/memory.h>

void yield();

#define SYSCALL_READ	0
#define SYSCALL_WRITE	1
#define SYSCALL_OPEN	2
#define SYSCALL_CLOSE	3
#define SYSCALL_MMAP	9
#define SYSCALL_INFO	99
#define SYSCALL_YIELD	24

uint64_t __isr_syscall(syscall_in *in)
{
	switch (in->syscall_no) {
		case SYSCALL_READ:
			break;
		case SYSCALL_WRITE:
			kprintf("%s", (char *)in->first_param);
			break;
		case SYSCALL_OPEN:
			break;
		case SYSCALL_CLOSE:
			break;
		case SYSCALL_MMAP:
			break;
		case SYSCALL_YIELD:
			yield();
			break;
		case SYSCALL_INFO:
			kprintf("SBUnix x86_64 OS\n");
			break;
		default:
			kprintf("Error: Unknown System Call\n");
			break;
	}
	return 0;
}
