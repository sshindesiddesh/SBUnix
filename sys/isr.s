.globl isr32
isr32:
    cli
    mov (%rsp), %rdi
    call    __isr_timer_cb
    sti
    iretq
