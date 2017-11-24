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
		"rax", "rdi", "rsi", "rdx"
	);
	return out;
}

int execvpe(const char *file, char *const argv[], char *const envp[])
{
	char cmd[100];
	size_t k = 0;
	int ret = -1;
	/* Execute without addding any path. */
	execve(file, argv, envp);
	/* Execute with appending paths */
	/* TODO : check if this can be checked against pointer. */
	while (env_p[k][0]) {
		strcpy(cmd, env_p[k]);
		strcat(cmd, "/");
		strcat(cmd, file);
		ret = execve(cmd, argv, NULL);
		k++;
	}
	return ret;
}
