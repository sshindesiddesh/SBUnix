#include <sys/kprintf.h>
#include <sys/syscall.h>
#include <sys/memory.h>
#include <sys/kutils.h>
#include <sys/tarfs.h>

void yield();

#define SYSCALL_READ	0
#define SYSCALL_WRITE	1
#define SYSCALL_OPEN	2
#define SYSCALL_CLOSE	3
#define SYSCALL_MMAP	9
#define SYSCALL_CHDIR	80
#define SYSCALL_INFO	99
#define SYSCALL_YIELD	24
#define SYSCALL_FORK	57
#define SYSCALL_EXECVE	59
#define SYSCALL_DUP2	33
#define SYSCALL_GETCWD	79
#define SYSCALL_GETDENTS 78

int console_read(int fd, char *buf, uint64_t count);
uint64_t __isr_syscall(syscall_in *in)
{
	uint64_t out = 0;
	switch (in->syscall_no) {
		case SYSCALL_READ:
			out = sys_read(in->first_param, (char *)in->second_param, in->third_param);		
			break;
		case SYSCALL_WRITE:
			kprintf("%s", (char *)in->second_param);
			out = strlen((char *)in->second_param);
			break;
		case SYSCALL_OPEN:
			out = tarfs_open((char *)in->first_param, in->second_param);
			break;
		case SYSCALL_CLOSE:
			out = tarfs_close(in->first_param);
			break;
		case SYSCALL_CHDIR:
			out = tarfs_chdir((char *)in->first_param);
			break;
		case SYSCALL_OPENDIR:
			out = tarfs_opendir((char *)in->first_param);
		case SYSCALL_MMAP:
			break;
		case SYSCALL_YIELD:
			yield();
			break;
		case SYSCALL_INFO:
			kprintf("SBUnix x86_64 OS\n");
			break;
		case SYSCALL_GETCWD:
			out = (uint64_t)tarf_getcwd((char *)in->first_param, (size_t)in->second_param);
			break;
		case SYSCALL_FORK:
			break;
		case SYSCALL_EXECVE:
			break;
		case SYSCALL_DUP2:
			break;
		default:
			kprintf("Error: Unknown System Call\n");
			break;
	}
	return out;
}

uint64_t sys_read(uint64_t fd_cnt, void *buf, uint64_t length)
{
	uint64_t count;
	if (fd_cnt == STD_IN) {
		count = console_read(fd_cnt , buf, length);
		return count;
	}
	else {
		count = tarfs_read(fd_cnt , buf, length);
		kprintf("buf : %s", buf);
		return count;
	}
	return -1;
}
