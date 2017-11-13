.text

.global __switch_ring3
__switch_ring3:
	cli

	mov $0x23, %rax
	mov %rax, %ds
	mov %rax, %es
	mov %rax, %fs
	mov %rax, %gs
	
	movq 8(%rdi), %rax
	pushq $0x23
	pushq %rax
	pushfq
	popq %rax
	orq $0x200, %rax
	pushq %rax
	pushq $0x1B
	pushq %rsi

	movq %rsp, %rdi
	callq set_tss_rsp

	sti
	iretq
