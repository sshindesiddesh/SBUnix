#include <sys/kprintf.h>
#include <sys/syscall.h>
#include <sys/memory.h>
#include <sys/kutils.h>
#include <sys/tarfs.h>
#include <sys/ksyscall.h>
#include <unistd.h>


#define SYSCALL_READ		0
#define SYSCALL_WRITE		1
#define SYSCALL_OPEN		2
#define SYSCALL_CLOSE		3
#define SYSCALL_MMAP		9
#define SYSCALL_MUNMAP		11
#define SYSCALL_BRK		12
#define SYSCALL_YIELD		24
#define SYSCALL_DUP2		33
#define SYSCALL_SLEEP		35
#define SYSCALL_GETPID		39
#define SYSCALL_SHUTDOWN	48
#define SYSCALL_FORK		57
#define SYSCALL_EXECVE		59
#define SYSCALL_EXIT		60
#define SYSCALL_WAIT		61
#define SYSCALL_KILL		62
#define SYSCALL_GETCWD		79
#define SYSCALL_GETDENTS	78
#define SYSCALL_CHDIR		80
#define SYSCALL_INFO		99
#define SYSCALL_GETPPID		110
#define SYSCALL_OPENDIR		300
#define SYSCALL_CLOSEDIR 	301
#define SYSCALL_READDIR		302
#define SYSCALL_PS		303

void print_params(syscall_in *in)
{
	kprintf("s no %x\n", in->in_out);
	kprintf("1p %x\n", in->first_param);
	kprintf("2p %x\n", in->second_param);
	kprintf("3p %x\n", in->third_param);
	kprintf("4p %x\n", in->fourth_param);
	kprintf("5p %x\n", in->fifth_param);
	kprintf("6p %x\n", in->sixth_param);
}

extern pcb_t *cur_pcb;
int putchar(int c);
int console_read(int fd, char *buf, uint64_t count);
uint64_t __isr_syscall(syscall_in *in)
{
#if 0
	if (in->in_out != 0 && in->in_out != 1) {
		kprintf("syscall %d %d\n", in->in_out, cur_pcb->pid);
	}
#endif
	uint64_t out = 0;
	switch (in->in_out) {
		case SYSCALL_READ:
			out = kread(in->first_param, (char *)in->second_param, in->third_param);
			break;
		case SYSCALL_WRITE:
			out = kwrite(in->first_param, (char *)in->second_param, in->third_param);
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
		case SYSCALL_GETDENTS:
			out = tarfs_getdents(in->first_param, (uint64_t)in->second_param, in->third_param);
			break;
		case SYSCALL_OPENDIR:
			out = (uint64_t)tarfs_opendir_user((char *)in->first_param, (dir_t *)in->second_param);
			break;
		case SYSCALL_CLOSEDIR:
			out = (uint64_t)tarfs_closedir_user((DIR *)in->first_param);
			break;
		case SYSCALL_READDIR:
			out = (uint64_t)tarfs_readdir_user((uint64_t *)in->first_param, (struct dirent *)in->second_param);
			break;
		case SYSCALL_MMAP:
			out = kmmap(in->first_param, in->second_param, in->third_param, in->fourth_param);
			break;
		case SYSCALL_MUNMAP:
			out = kmunmap(in->first_param, in->second_param);
			break;
		case SYSCALL_YIELD:
			kyield();
			break;
		case SYSCALL_INFO:
			kprintf("SBUnix x86_64 OS\n");
			break;
		case SYSCALL_GETCWD:
			out = (uint64_t)tarfs_getcwd((char *)in->first_param, (size_t)in->second_param);
			break;
		case SYSCALL_EXECVE:
			kexecve((char *)in->first_param, (char **)in->second_param, (char **)in->third_param);
			break;
		case SYSCALL_WAIT:
			out = kwait((pid_t)in->first_param);
			break;
		case SYSCALL_DUP2:
			break;
		case SYSCALL_FORK:
			out = kfork();
			break;
		case SYSCALL_BRK:
			out = kbrk((uint64_t)in->first_param);
			break;
		case SYSCALL_EXIT:
			kexit(in->first_param);
			break;
		case SYSCALL_GETPID:
			out = kgetpid();
			break;
		case SYSCALL_GETPPID:
			out = kgetppid();
			break;
		case SYSCALL_PS:
			kps();
			break;
		case SYSCALL_KILL:
			kkill(in->first_param);
			break;
		case SYSCALL_SLEEP:
			ksleep(in->first_param);
			break;
		case SYSCALL_SHUTDOWN:
			kshutdown();
			break;
		default:
			kprintf("Error: Unknown System Call\n");
			break;
	}
	__flush_tlb();
	/* Set the output value in rax */
	in->in_out = out;
	return out;
}

uint64_t kread(uint64_t fd, void *buf, uint64_t length)
{
	uint64_t count;
	if (fd == STD_IN) {
		count = console_read(fd , buf, length);
#if 0
		kprintf("READ: %p %s\n", buf, buf);
#endif
		return count;
	} else {
		count = tarfs_read(fd , buf, length);
		return count;
	}
	return count;
}

uint64_t kwrite(uint64_t fd, char *buf, int length)
{
	if (fd == STD_OUT || fd == STD_ERR) {
		uint64_t count, i = 0;
		if (length > strlen((char *)buf))
			length = strlen((char *)buf);
		count = length;
		while (length >= 0) {
			putchar(buf[i++]);
			length--;
		}
		return count;
	}
	return 0;
}

uint64_t kbrk(uint64_t npages)
{
	uint64_t address, size;
	size = npages*PG_SIZE;
	address = cur_pcb->mm->brk;

	mm_struct_t *mm = cur_pcb->mm;
	if(mm) {
		mm->brk += size;
		mm->data_end += size;
		mm->t_vm += size;
#if 0
		kprintf("returning from kbrk, address %p, mm->brk %p", address, mm->brk);
#endif
		return address;
	}
	else
	return -1;
}
