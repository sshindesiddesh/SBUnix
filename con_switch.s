.text

.global __context_switch
__context_switch:
	push rdi
	mov rsp, (rdi)
	mov (rsi), rsp
	pop rdi
	retq
