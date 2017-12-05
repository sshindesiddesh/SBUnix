#include <stdlib.h>
#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>
#include <unistd.h>

#if 0
/* Set the Environment Variable ponter from stack */
void init_ep(char *env[])
{
	if (!env)
		return;
	int i = 0;
	while (env[i]) {
		strcpy(usr_env[usr_env_cnt], env[i]);
		usr_env_p[usr_env_cnt] = env[i];
		usr_env_cnt++;
		usr_env[usr_env_cnt] = NULL;
	}
}
#endif

extern char *usr_env_p[50];
void _start() {
	int i;
	for(i = 0; i < 50; i++) {
		usr_env_p[i] = NULL;
	}

#if 0
	char cur_path[50];
	getcwd(cur_path, sizeof(cur_path));
	setenv("PATH", cur_path);
#endif

	/* Move value pointed by rsp to rdi which is the first argument to a funcion */
	__asm("movq (%rsp), %rdi");
	/* Move value in rsp to rsi which is the second argument to a funcion */
	__asm("movq %rsp, %rsi");
	/* Add 8 to RSP and store it in RSI */
	__asm("movq $8, %rax");
	__asm("addq %rax, %rsi");
	/* Get the env pointer from ((argc * 0x8) + 0x10)*/
	__asm("movq %rdi, %rdx");
	__asm("movq $8, %rax");
	__asm("imulq %rax, %rdx");
	__asm("movq $0x10, %rax");
	__asm("addq %rax, %rdx");
	__asm("addq %rsp, %rdx");
	/* Main function */
	__asm("call main");
	/* Exit system call */
	__asm("movq $60, %rax");
	__asm("int $0x80");
}
