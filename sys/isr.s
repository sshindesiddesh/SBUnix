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
    call push_all
    call    __isr_timer_cb
    call pop_all
    sti
    iretq

/* Keyboard ISR */
.global isr21
isr21:
    cli
    call push_all
    call    __isr_keyboard_cb
    call pop_all
    sti
    iretq

/* Syscall */
.global isr80
isr80:
	cli
	pushq %rbp
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	call    __isr_syscall
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rbp
	sti
	iretq
