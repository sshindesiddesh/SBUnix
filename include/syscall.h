#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <sys/defs.h>

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
#define SYSCALL_GETDENTS	78
#define SYSCALL_GETCWD		79
#define SYSCALL_CHDIR		80
#define SYSCALL_INFO		99
#define SYSCALL_GETPPID		110
#define SYSCALL_OPENDIR		300
#define SYSCALL_CLOSEDIR 	301
#define SYSCALL_READDIR		302
#define SYSCALL_PS		303

static inline uint64_t __libc_syscall__0(uint64_t sys_no)
{
	uint64_t out;
	__asm__ __volatile__ (
		"movq %[no], %%rax;"
		"int $0x80;"
		: "=a" (out)
		: [no]"a"(sys_no)
		: "%rbx", "%rcx", "%rdx", "%rbp", "%rsi", "%rdi", "%r12", "%r13", "%r14", "%r15"
	);
	return out;
}

static inline uint64_t __libc_syscall__1(uint64_t sys_no, uint64_t input_1)
{
	uint64_t out;
	__asm__ __volatile__ (
		"movq %[no], %%rax;"
		"movq %[input_1], %%rdi;"
		"int $0x80;"
		: "=a" (out)
		: [no]"a"(sys_no), [input_1]"g"(input_1)
		: "%rbx", "%rcx", "%rdx", "%rbp", "%rsi", "%rdi", "%r12", "%r13", "%r14", "%r15"
	);
	return out;
}

static inline uint64_t __libc_syscall__2(uint64_t sys_no, uint64_t input_1, uint64_t input_2)
{
	uint64_t out;
	__asm__ __volatile__ (
		"movq %[no], %%rax;"
		"movq %[input_1], %%rdi;"
		"movq %[input_2], %%rsi;"
		"int $0x80;"
		: "=a" (out)
		: [no]"a"(sys_no), [input_1]"g"(input_1), [input_2]"g"(input_2)
		: "%rbx", "%rcx", "%rdx", "%rbp", "%rsi", "%rdi", "%r12", "%r13", "%r14", "%r15"
	);
	return out;
}

static inline uint64_t __libc_syscall__3(uint64_t sys_no, uint64_t input_1, uint64_t input_2, uint64_t input_3)
{
	uint64_t out;
	__asm__ __volatile__ (
		"movq %[no], %%rax;"
		"movq %[input_1], %%rdi;"
		"movq %[input_2], %%rsi;"
		"movq %[input_3], %%rdx;"
		"int $0x80;"
		: "=a" (out)
		: [no]"a"(sys_no), [input_1]"g"(input_1), [input_2]"g"(input_2), [input_3]"g"(input_3)
		: "%rbx", "%rcx", "%rdx", "%rbp", "%rsi", "%rdi", "%r12", "%r13", "%r14", "%r15"
	);
	return out;
}

#endif
