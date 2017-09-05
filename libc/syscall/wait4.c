#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pid_t waitpid(pid_t pid, int *status, int options)
{
	pid_t out;

	__asm__ (
		/* System Call Number */
		"movq $61, %%rax\n"
		/* Param 1 */
		"movq %1, %%rdi;\n"
		/* Param 2 */
		"movq %2, %%rsi\n;"
		/* Param 3 */
		"movq %3, %%rdx\n;"
		/* Param 4 */
		"movq $0, %%rcx\n;"
#if 0
		/* Param 5 */
		"movq $0, %%r8\n"
		/* Param 6 */
		"movq %1, %%r9\n"
#endif
		"syscall\n"
		/* Output of the system call */
		"movq %%rax, %0\n"
		: "=m"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		"m" (pid), "m" (status), "m"(options)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rdi", "rsi", "rdx", "rcx"
	);

	putc('A', stdout);
	return out;
}
