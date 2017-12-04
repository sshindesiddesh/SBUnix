/* Timer ISR  */
.global isr20
isr20:
	cli
	pushq %rax
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rdi
	pushq %rsi
	pushq %rbp
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	callq __isr_timer_cb
	callq pre_empt_yield
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rsi
	popq %rdi
	popq %rbp
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax
	//sti
	iretq

/* Keyboard ISR */
.global isr21
isr21:
	cli
	pushq %rax
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rbp
	pushq %rdi
	pushq %rsi
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	callq __isr_keyboard_cb
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rsi
	popq %rdi
	popq %rbp
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax
	//sti
	iretq

/* Syscall */
.global isr80
isr80:
	cli
	/* 14 registers */
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rbp
	pushq %rdi
	pushq %rsi
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	/* 7 input params */
	pushq %rax
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %r10
	pushq %r8
	pushq %r9
	movq %rsp, %rdi
	callq __isr_syscall
	addq $48, %rsp
	popq %rax

	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rsi
	popq %rdi
	popq %rbp
	popq %rdx
	popq %rcx
	popq %rbx
	//sti
	iretq

.global excpE
excpE:
	cli
	pushq %rsi
	movq 8(%rsp), %rsi
	pushq %rax
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rbp
	pushq %rdi
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	movq %cr2, %rdi
	call __page_fault_handler
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rdi
	popq %rbp
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax
	popq %rsi
	/* There is extra push for error no in the page fault handler */
	addq $8, %rsp
	//sti
	iretq
