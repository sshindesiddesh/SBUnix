#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>
#include <sys/idt.h>

IDTDesc idt_desc[NO_OF_INT];
idtr_t idtr;

void _x86_64_asm_lidt (idtr_t* idtr);

/* Function to Initialize memory */
void *memset(void *ptr, int value, size_t len)
{
	uint8_t *p = (uint8_t *)ptr;
	while (len--)
		*p++ = value;
	return ptr;
}

/* Populate IDT entry as per ID */
void idt_populate_desc(uint8_t id, uint64_t int_handler)
{
	IDTDesc *idt_desc_p = &idt_desc[id];
	
	/* RPL:00, GDT:0, CS INDEX:1 */
	idt_desc_p->selector = 0x08;
	uint64_t addr = (uint64_t) int_handler;
	idt_desc_p->offset_1 = addr & 0xFFFF;
	idt_desc_p->offset_2 = (addr >> 16) & 0xFFFF;
	idt_desc_p->offset_3 = (addr >> 32) & 0xFFFFFFFF;
	/* Interrupt Present and Interrupt 32 bit gate */
	idt_desc_p->type_attr = 0x8E;
	/* Single Kernel Stack */
	idt_desc_p->ist = 0;
}

void init_idt()
{
	/* Set the idtr memory to 0*/
	memset(idt_desc, 0, sizeof(idt_desc));
	
	idt_populate_desc(0x0, (uint64_t)excp0);
	idt_populate_desc(0x1, (uint64_t)excp1);
	idt_populate_desc(0x2, (uint64_t)excp2);
	idt_populate_desc(0x3, (uint64_t)excp3);

	idt_populate_desc(0x4, (uint64_t)excp4);
	idt_populate_desc(0x5, (uint64_t)excp5);
	idt_populate_desc(0x6, (uint64_t)excp6);
	idt_populate_desc(0x7, (uint64_t)excp7);

	idt_populate_desc(0x8, (uint64_t)excp8);
	idt_populate_desc(0x9, (uint64_t)excp9);
	idt_populate_desc(0xA, (uint64_t)excpA);
	idt_populate_desc(0xB, (uint64_t)excpB);

	idt_populate_desc(0xC, (uint64_t)excpC);
	idt_populate_desc(0xD, (uint64_t)excpD);
	idt_populate_desc(0xE, (uint64_t)excpE);
	idt_populate_desc(0xF, (uint64_t)excpF);

	idt_populate_desc(0x10, (uint64_t)excp10);
	idt_populate_desc(0x11, (uint64_t)excp11);
	idt_populate_desc(0x12, (uint64_t)excp12);
	idt_populate_desc(0x13, (uint64_t)excp13);

	idt_populate_desc(0x14, (uint64_t)excp14);
	idt_populate_desc(0x15, (uint64_t)excp15);
	idt_populate_desc(0x16, (uint64_t)excp16);
	idt_populate_desc(0x17, (uint64_t)excp17);

	idt_populate_desc(0x18, (uint64_t)excp18);
	idt_populate_desc(0x19, (uint64_t)excp19);
	idt_populate_desc(0x1A, (uint64_t)excp1A);
	idt_populate_desc(0x1B, (uint64_t)excp1B);

	idt_populate_desc(0x1C, (uint64_t)excp1C);
	idt_populate_desc(0x1D, (uint64_t)excp1D);
	idt_populate_desc(0x1E, (uint64_t)excp1E);
	idt_populate_desc(0x1F, (uint64_t)excp1F);
	/* Timer Interrupt */
	idt_populate_desc(IRQ0, (uint64_t)isr20);
	/* Keyboard Interrupt */
	idt_populate_desc(IRQ1, (uint64_t)isr21);

	/* Syscall Handler */
	idt_populate_desc(0x80, (uint64_t)isr80);

	idtr.address = (uint64_t)idt_desc;
	/* TODO: Check for this -1 */
	idtr.len = sizeof(idt_desc);
	_x86_64_asm_lidt(&idtr);
}
