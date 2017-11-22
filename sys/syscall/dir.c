#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

uint64_t opendir(const char *pathname)
{
	uint64_t out;
	__asm__ (
		/* System Call Number */
		"movq $300, %%rax\n"
		/* Param 1 */
		"movq %1, %%rdi\n"

		"int $0x80\n"
		/* Output of the system call */
		"movq %%rax, %0\n"
		: "=m"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		"m" (pathname)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rdi"
	);
	
	return out;
}

struct dirent *readdir(uint64_t dir)
{
	struct dirent *out;
	__asm__ (
		/* System Call Number */
		"movq $302, %%rax\n"
		/* Param 1 */
		"movq %1, %%rdi\n"
		/* Param 2 */
		/* "movq %2, %%rsi\n"*/

		"int $0x80\n"
		/* Output of the system call */
		"movq %%rax, %0\n"
		: "=m"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		"m" (dir)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rdi", "rsi"
	);
	
	return out;
}

uint64_t closedir(uint64_t dir)
{
	uint64_t out;
	__asm__ (
		/* System Call Number */
		"movq $300, %%rax\n"
		/* Param 1 */
		"movq %1, %%rdi\n"

		"int $0x80\n"
		/* Output of the system call */
		"movq %%rax, %0\n"
		: "=m"(out)/* output parameters, we aren't outputting anything, no none */
		/* (none) */
		: /* input parameters mapped to %0 and %1, repsectively */
		"m" (dir)
		: /* registers that we are "clobbering", unneeded since we are calling exit */
		"rax", "rdi"
	);
	return out;
}	

