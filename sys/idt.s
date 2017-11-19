.text

.global _x86_64_asm_lidt
_x86_64_asm_lidt:
	lidt (%rdi)
	retq

.global __flush_tlb
__flush_tlb:
	pushq %rax
	movq %cr3, %rax
	movq %rax, %cr3
	popq %rax
	retq
