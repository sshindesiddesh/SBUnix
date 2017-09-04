#include <stdlib.h>
#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>

void _start(void) {
	__asm(
	/* Move value pointed by rsp to rdi which is the first argument to a funcion */
		"movq (%%rsp), %%rdi\n"
	/* Move address in rsp + 8 to rsi which is the second argument to a funcion */
		"movq 8(%%rsp), %%rsi\n"
	/* Main function */
		"call main\n"
	/* Exit system call */
		"movq $60, %%rax\n"
		"syscall\n"
		:
		:
		: "rdi", "rsi", "rsp", "rax"
	);
}
