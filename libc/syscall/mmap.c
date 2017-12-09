#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

uint64_t mmap(uint64_t va_start, uint64_t size, uint64_t flags, uint64_t type)
{
	size_t out;
	__asm__ (
		/* System Call Number */
		"movq $9, %%rax\n"
		/* Param 1 */
		"movq %1, %%rdi\n"
		/* Param 2 */
		"movq %2, %%rsi\n"
		/* Param 3 */
		"movq %3, %%rdx\n"
		/* Param 4 */
		"movq %4, %%r10\n"
#if 0
		/* Param 5 */
		"movq %1, %%r8\n"
		/* Param 6 */
		"movq %1, %%r9\n"
		"syscall\n"
#endif
		"int $0x80\n"
		/* Output of the system call */
		"movq %%rax, %0\n"
		: "=r"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		"r" (va_start), "r" (size), "r" (flags), "r" (type)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rdi", "rsi", "rdx", "r10"
	);
	return out;
}
