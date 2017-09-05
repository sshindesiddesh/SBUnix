#include <stdlib.h>
#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>

void _start() {
	/* Move value pointed by rsp to rdi which is the first argument to a funcion */
	__asm("movq (%rsp), %rdi");
	/* Move value in rsp to rsi which is the second argument to a funcion */
	__asm("movq %rsp, %rsi");
	/* Add 8 to RSP and store it in RSI */
	__asm("movq $8, %rax");
	__asm("addq %rax, %rsi");
	/* Main function */
	__asm("call main");
	/* Exit system call */
	__asm("movq $60, %rax");
	__asm("syscall");
}
