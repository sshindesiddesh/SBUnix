#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

uint64_t shutdown(void)
{
	uint64_t out;
	__asm__ (
		/* System Call Number */
		"movq $48, %%rax;"
#if 0
		/* Param 1 */
		"movq %1, %%rdi;"
		/* Param 2 */
		"movq %2, %%rsi;"
		/* Param 3 */
		"movq %3, %%rdx\n"
		/* Param 4 */
		"movq %1, %%rcx\n"
		/* Param 5 */
		"movq %1, %%r8\n"
		/* Param 6 */
		"movq %1, %%r9\n"
		"syscall;"
#endif
		"int $0x80\n"
		/* Output of the system call */
		"movq %%rax, %0;"
		: /* output parameters, we aren't outputting anything, no none */
		"=r" (out)
		: /* input parameters mapped to %0 and %1, repsectively */
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rbx", "rcx", "rdx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "rbp"
	);
	return out;
}
