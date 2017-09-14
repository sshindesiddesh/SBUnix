#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>

#define NO_OF_INT	256


#define IRQ0	0x20
#define IRQ1	0x21
#define IRQ2	0x22
#define IRQ3	0x23
#define IRQ4	0x24
#define IRQ5	0x25
#define IRQ6	0x26
#define IRQ7	0x27

#define IRQ8	0x28
#define IRQ9	0x29
#define IRQ10	0x2A
#define IRQ11	0x2B
#define IRQ12	0x2C
#define IRQ13	0x2D
#define IRQ14	0x2E
#define IRQ15	0x2F


typedef struct IDTDesc {
	uint16_t offset_1; // offset bits 0..15
	/* 15:3 -> Index 2, 2 -> GDT/LDT, 1:0 -> RPL (Request Previlege Level) */
	uint16_t selector; // a code segment selector in GDT or LDT
	uint8_t ist;       // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
	/* P:DPL:S:GATE-TYPE */
	uint8_t type_attr; // type and attributes
	uint16_t offset_2; // offset bits 16..31
	uint32_t offset_3; // offset bits 32..63
	uint32_t zero;     // reserved
} __attribute__((packed)) IDTDesc;

typedef struct IDTR {
	uint16_t len;		//Length
	uint64_t address;	//Address
} __attribute__((packed)) idtr_t;

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

#define WEAK  __attribute__((weak))
//void isr20() {}
WEAK void isr20(void);
WEAK void isr21(void);
WEAK void isr22(void);
WEAK void isr23(void);
WEAK void isr24(void);
WEAK void isr25(void);
WEAK void isr26(void);
WEAK void isr27(void);

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
	
	/* Timer Interrupt */
	idt_populate_desc(IRQ0, (uint64_t)isr20);
	//idt_populate_desc(IRQ1, (uint64_t)isr21);

	idtr.address = (uint64_t)idt_desc;
	/* TODO: Check for this -1 */
	idtr.len = sizeof(idt_desc);
	_x86_64_asm_lidt(&idtr);
}
