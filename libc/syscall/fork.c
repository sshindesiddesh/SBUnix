#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pid_t fork()
{
	int64_t out;
	__asm__ (
		/* System Call Number */
		"movq $57, %%rax\n"
#if 0
		/* Param 1 */
		"movq %1, %%rdi\n"
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
#endif
		"int $0x80\n"
		/* Output of the system call */
		"movq %%rax, %0\n"
		: "=r"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax"
	);
	return out;
}
