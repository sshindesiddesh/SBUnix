#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int open(const char *pathname, int flags, uint64_t mode)
{
	size_t out;
	__asm__ (
		/* System Call Number */
		"movq $2, %%rax\n"
		/* Param 1 */
		"movq %1, %%rdi\n"
		/* Param 2 */
		"movq %2, %%rsi\n"
		/* Param 3 */
		"movq %3, %%rdx\n"
#if 0
		/* Param 4 */
		"movq %1, %%rcx\n"
		/* Param 5 */
		"movq %1, %%r8\n"
		/* Param 6 */
		"movq %1, %%r9\n"
		"syscall\n"
#endif
		"int $0x80\n"
		/* Output of the system call */
		"movq %%rax, %0\n"
		: "=m"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		"m" (pathname), "m" (flags), "m" (mode)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rdi", "rsi", "rdx"
	);
	return out;
}

int close(int fd)
{
	size_t out;
	__asm__ (
		/* System Call Number */
		"movq $3, %%rax\n"
		/* Param 1 */
		"movq %1, %%rdi\n"
#if 0
		/* Param 2 */
		"movq %2, %%rsi\n"
		/* Param 3 */
		"movq %3, %%rdx\n"
		/* Param 4 */
		"movq %1, %%rcx\n"
		/* Param 5 */
		"movq %1, %%r8\n"
		/* Param 6 */
		"movq %1, %%r9\n"
		"syscall\n"
#endif
		"int $0x80\n"
		/* Output of the system call */
		"movq %%rax, %0\n"
		: "=m"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		"m" (fd)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rdi"
	);

	return out;
}

