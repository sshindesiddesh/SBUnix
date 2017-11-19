#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>
#include <sys/idt.h>
#include <sys/kutils.h>
#include <sys/memory.h>
#include <sys/process.h>

/* Head of the running linked list for yield */
extern pcb_t *cur_pcb;

IDTDesc idt_desc[NO_OF_INT];
idtr_t idtr;

void _x86_64_asm_lidt (idtr_t* idtr);

/* Populate IDT entry as per ID */
void idt_populate_desc(uint8_t id, uint64_t int_handler, uint8_t dpl)
{
	IDTDesc *idt_desc_p = &idt_desc[id];
	
	/* RPL:00, GDT:0, CS INDEX:1 */
	idt_desc_p->selector = 0x08;
	uint64_t addr = (uint64_t) int_handler;
	idt_desc_p->offset_1 = addr & 0xFFFF;
	idt_desc_p->offset_2 = (addr >> 16) & 0xFFFF;
	idt_desc_p->offset_3 = (addr >> 32) & 0xFFFFFFFF;
	/* Interrupt Present and Interrupt 32 bit gate */
	idt_desc_p->type_attr = 0x8E | (dpl << 5);
	/* Single Kernel Stack */
	idt_desc_p->ist = 0;
}

va_t pa2va(pa_t pa);

pa_t va2pa(va_t va)
{
	return (va - (va_t)KERNBASE);
}

extern pml_t *pml;
extern uint64_t phys_base;
extern uint64_t phys_free;
extern uint64_t phys_end;
void __page_fault_handler(uint64_t faultAddr, uint64_t err_code)
{
#ifdef	VMA_DEBUG
	kprintf("!!!Pagefault : Address: %p Error %x !!!\n", faultAddr, err_code);
#endif
	vma_t *vma = check_addr_in_vma_list(faultAddr, cur_pcb->mm->head);
	if (!vma) {
		kprintf("!!!Segmentation Fault!!!");
		while (1);
	}
	if (vma->type == HEAP) {
		allocate_vma(cur_pcb, vma);
	} else if (vma->type == STACK) {

	}
}

void init_idt()
{
	/* Set the idtr memory to 0*/
	memset(idt_desc, 0, sizeof(idt_desc));
	
	idt_populate_desc(0x0, (uint64_t)excp0, 0);
	idt_populate_desc(0x1, (uint64_t)excp1, 0);
	idt_populate_desc(0x2, (uint64_t)excp2, 0);
	idt_populate_desc(0x3, (uint64_t)excp3, 0);

	idt_populate_desc(0x4, (uint64_t)excp4, 0);
	idt_populate_desc(0x5, (uint64_t)excp5, 0);
	idt_populate_desc(0x6, (uint64_t)excp6, 0);
	idt_populate_desc(0x7, (uint64_t)excp7, 0);

	idt_populate_desc(0x8, (uint64_t)excp8, 0);
	idt_populate_desc(0x9, (uint64_t)excp9, 0);
	idt_populate_desc(0xA, (uint64_t)excpA, 0);
	idt_populate_desc(0xB, (uint64_t)excpB, 0);

	idt_populate_desc(0xC, (uint64_t)excpC, 0);
	idt_populate_desc(0xD, (uint64_t)excpD, 0);
	idt_populate_desc(0xE, (uint64_t)excpE, 0);
	idt_populate_desc(0xF, (uint64_t)excpF, 0);

	idt_populate_desc(0x10, (uint64_t)excp10, 0);
	idt_populate_desc(0x11, (uint64_t)excp11, 0);
	idt_populate_desc(0x12, (uint64_t)excp12, 0);
	idt_populate_desc(0x13, (uint64_t)excp13, 0);

	idt_populate_desc(0x14, (uint64_t)excp14, 0);
	idt_populate_desc(0x15, (uint64_t)excp15, 0);
	idt_populate_desc(0x16, (uint64_t)excp16, 0);
	idt_populate_desc(0x17, (uint64_t)excp17, 0);

	idt_populate_desc(0x18, (uint64_t)excp18, 0);
	idt_populate_desc(0x19, (uint64_t)excp19, 0);
	idt_populate_desc(0x1A, (uint64_t)excp1A, 0);
	idt_populate_desc(0x1B, (uint64_t)excp1B, 0);

	idt_populate_desc(0x1C, (uint64_t)excp1C, 0);
	idt_populate_desc(0x1D, (uint64_t)excp1D, 0);
	idt_populate_desc(0x1E, (uint64_t)excp1E, 0);
	idt_populate_desc(0x1F, (uint64_t)excp1F, 0);
	/* Timer Interrupt */
	idt_populate_desc(IRQ0, (uint64_t)isr20, 0);
	/* Keyboard Interrupt */
	idt_populate_desc(IRQ1, (uint64_t)isr21, 0);

	/* Syscall Handler */
	idt_populate_desc(0x80, (uint64_t)isr80, 3);

	idtr.address = (uint64_t)idt_desc;
	/* TODO: Check for this -1 */
	idtr.len = sizeof(idt_desc);
	_x86_64_asm_lidt(&idtr);
}
