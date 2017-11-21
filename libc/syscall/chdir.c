#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int chdir(const char *path)
{
	size_t out;
	__asm__ (
		/* system call number */
		"movq $80, %%rax\n"
		/* param 1 */
		"movq %1, %%rdi\n"
#if 0
		/* param 2 */
		"movq %2, %%rsi\n"
		/* param 3 */
		"movq %3, %%rdx\n"
		/* param 4 */
		"movq %1, %%rcx\n"
		/* param 5 */
		"movq %1, %%r8\n"
		/* param 6 */
		"movq %1, %%r9\n"
		"syscall\n"
#endif
		"int $0x80\n"
		/* output of the system call */
		"movq %%rax, %0\n"
		: "=m"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		"m" (path)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rdi"
	);
	return out;
}
