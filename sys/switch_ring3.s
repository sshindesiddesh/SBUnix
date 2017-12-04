.text

.global __switch_ring3
__switch_ring3:
	cli
	/* Input user stack pointer to rax */
	movq 0x20(%rdi), %rax
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
	pushq 0x18(%rdi)
	/* Switching CR3 still makes kernel accessible as we are yet in ring0.
	 * After iretq we will return to ring 3 where these pages will be inaccessible. */
	movq 0x10(%rdi), %rax
	movq %rax, %cr3
	//sti
	iretq
