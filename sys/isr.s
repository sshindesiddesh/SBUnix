/* Push all */
.global push_all
push_all:
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

/* Pop all */
.global pop_all
pop_all:
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

/* Timer ISR  */
.global isr20
isr20:
    cli
    call    __isr_timer_cb
    call    __syscall_yield
    sti
    iretq

/* Keyboard ISR */
.global isr21
isr21:
    cli
    call    __isr_keyboard_cb
    sti
    iretq

/* Syscall */
.global isr80
isr80:
	cli
	pushq %rax
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %r10
	pushq %r8
	pushq %r9
	movq %rsp, %rdi
	call    __isr_syscall
	popq %r9
	popq %r8
	popq %r10
	popq %rdx
	popq %rsi
	popq %rdi
	popq %rax
	sti
	iretq

.global excpE
excpE:
	cli
	popq %rsi
	movq %cr2, %rdi
	call    __page_fault_handler
	sti
	iretq
