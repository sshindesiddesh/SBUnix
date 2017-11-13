/* Timer ISR  */
.global isr20
isr20:
    cli
    call    __isr_timer_cb
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
    call    __isr_syscall
    sti
    iretq
