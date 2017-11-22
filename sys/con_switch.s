.text

.global __context_switch
__context_switch:
	cli

	pushq %rdi
	movq %rsp, 8(%rdi)
	movq 8(%rsi), %rsp
	popq %rdi

	sti
	retq
