#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/defs.h>

int waitpid(int pid, int *status)
{
	int64_t out;
	__asm__ (
		/* System Call Number */
		"movq $61, %%rax\n"
		/* Param 1 */
		"movq %1, %%rdi\n"
		/* Param 2 */
		"movq %2, %%rsi\n"
#if 0
		/* Param 3 */
		"movq %3, %%rdx\n"
		/* Param 4 */
		"movq %4, %%rcx\n;"
		/* Param 5 */
		"movq $0, %%r8\n"
		/* Param 6 */
		"movq %1, %%r9\n"
#endif
		"int $0x80\n"
		/* Output of the system call */
		"movq %%rax, %0\n"
		: "=r"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		"r" ((int64_t)pid), "r" ((int64_t)status)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rdi", "rsi"
	);
	return out;
}

pid_t wait(int *status)
{
	return waitpid(0, status);
}
