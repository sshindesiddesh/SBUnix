.text

.global __context_switch
__context_switch:
	cli
	pushq %rax
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rbp

	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	/* pushq %rdi
	movq 8(%rsi), %rdi
	callq set_tss_rsp
	popq %rdi
	*/

	pushq %rdi
	movq %rsp, 8(%rdi)
	movq 8(%rsi), %rsp
	popq %rdi

	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8

	popq %rbp
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax

	sti
	retq
