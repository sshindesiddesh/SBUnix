.text

.global __switch_ring3
__switch_ring3:

	cli

	movq $0x23, %rax
	movq %rax, %ds
	movq %rax, %es
	movq %rax, %fs
	movq %rax, %gs

	/* Input user stack pointer to rax */
	movq %rdi, %rax

	/* SS */
	pushq $0x23
	/* User RSP */
	pushq %rax
	/* EFLAGS */
	pushfq
	popq %rbx
	orq $0x200, %rbx
	pushq %rbx
	/* CS */
	pushq $0x2B
	/* Input entry pointer */
	pushq %rsi

	sti

	iretq
