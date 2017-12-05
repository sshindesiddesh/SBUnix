#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern char env_p[30][100];

int execve(const char *file, char *const argv[], char *const envp[])
{
	size_t out;
	__asm__ (
		/* System Call Number */
		"movq $59, %%rax\n"
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
#endif
		"int $0x80\n"
		/* Output of the system call */
		"movq %%rax, %0\n"
		: "=m"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		"m" (file), "m" (argv), "m" (envp)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rbx", "rcx", "rdx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "rbp"
	);
	return out;
}

extern char *usr_env_p[50];
int execvpe(const char *file, char *const argv[], char *const envp[])
{
	int ret = -1;
	ret = execve(file, argv, usr_env_p);
#if 0
	int i = 0;
	while (usr_env_p[i]) {
		printf("%s\n", usr_env_p[i++]);
	}
#endif
	return ret;
}
