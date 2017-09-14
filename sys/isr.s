
.globl isr20
isr20:
    cli
    ;mov 90(%rsp), %rdi
    call    __isr_timer_cb
    sti
    iretq
