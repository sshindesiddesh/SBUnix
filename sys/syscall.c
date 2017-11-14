#include <sys/kprintf.h>
#include <sys/pic.h>

void yield();

void __isr_syscall()
{
	uint64_t syscall_no;
	uint64_t second_param, third_param, fourth_param;
	__asm__ volatile (
		"movq %%rax, %0;"
		"movq %%rbx, %1;"
		"movq %%rcx, %2;"
		"movq %%rdx, %3;"
		: "=m"(syscall_no), "=m"(second_param), "=m"(third_param), "=m"(fourth_param)
		:
		: "rax", "rbx", "rcx", "rdx"
		);
	if (syscall_no == 1) {
		kprintf("%s", (char *)second_param);
	} else if (syscall_no == 2) {
		yield();
	} else {
		kprintf("Under Development\n");
	}
}
