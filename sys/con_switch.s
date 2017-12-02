.text

.global __context_switch
__context_switch:

	pushq %rdi
	movq %rsp, 8(%rdi)
	movq 8(%rsi), %rsp
	popq %rdi

	retq

.global __exit_switch
__exit_switch:
	cli
	movq 8(%rdi), %rsp
	popq %rdi
	sti
	retq
