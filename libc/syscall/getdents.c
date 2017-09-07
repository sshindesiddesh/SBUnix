#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


//int getdents(unsigned int fd, struct dirent *dir, unsigned int count)
int getdents(unsigned int fd, char* dir, unsigned int count)
{
	int64_t out;
	__asm__ (
		/* System Call Number */
		"movq $78, %%rax;"
		/* Param 1 */
		"movq %1, %%rdi;"
		/* Param 2 */
		"movq %2, %%rsi;"
		/* Param 3 */
		"movq %3, %%rdx;"
#if 0
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
		: "=r"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		"r" ((uint64_t)fd), "r" (dir), "r" ((uint64_t)count)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rdi", "rsi", "rdx"
	);
	return out;
}