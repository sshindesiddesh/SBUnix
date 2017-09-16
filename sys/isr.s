
.global isr20
.global isr21
isr20:
    cli
    ;mov 90(%rsp), %rdi
    call    __isr_timer_cb
    sti
    iretq
isr21:
    cli
    ;mov 90(%rsp), %rdi
    call    __isr_keyboard_cb
    sti
    iretq
