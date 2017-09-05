#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *getcwd(char *buf, size_t size)
{
	char *out;
	__asm__ (
		/* System Call Number */
		"movq $79, %%rax;"
		/* Param 1 */
		"movq %1, %%rdi;"
		/* Param 2 */
		"movq %2, %%rsi;"
#if 0
		/* Param 3 */
		"movq %3, %%rdx\n"
		/* Param 4 */
		"movq %1, %%rcx\n"
		/* Param 5 */
		"movq %1, %%r8\n"
		/* Param 6 */
		"movq %1, %%r9\n"
#endif
		"syscall;"
		/* Output of the system call */
		"movq %%rax, %0;"
		: /* output parameters, we aren't outputting anything, no none */
		"=r" (out)
		: /* input parameters mapped to %0 and %1, repsectively */
		"r" (buf), "r" (size)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rdi", "rsi"
	);
	return out;
}
