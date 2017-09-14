#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>

#define PIC1	0x20
#define PIC2	0xA0
#define PIC1_CMD	PIC1
#define PIC2_CMD	PIC2
#define PIC1_DATA	(PIC1 + 1)
#define PIC2_DATA	(PIC2 + 1)

#define PIC_EOI	0x20

#define ICW1_ICW4	0x01
#define ICW1_INIT	0x10
#define ICW4_8086	0x01

static void outb(uint16_t port, uint8_t val)
{
	__asm__ __volatile__ ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

void iowait(void) {}

/* TODO: Remove this later. This is redeclared */
#define IRQ0	0x20
#define IRQ8	0x28


void pic_init()
{
	__asm__ __volatile__("cli");
	/* Initialize Master */
	outb(PIC1_CMD, ICW1_ICW4 | ICW1_INIT);
	iowait();
	/* Initialize Slave */
	outb(PIC2_CMD, ICW1_ICW4 | ICW1_INIT);
	iowait();
	/* Offset for Master is IRQ0 */
	outb(PIC1_DATA, IRQ0);
	iowait();
	/* Offset for Slave is IRQ8 */
	outb(PIC2_DATA, IRQ8);
	iowait();
	/* Inform Pic about wired cascade */
	/* ToDO: Change it to 4 */
	outb(PIC1_DATA, 0x00);
	iowait();
	/* ToDO: Change it to 2 */
	outb(PIC2_DATA, 0x00);
	iowait();
	/* 8086 mode */
	outb(PIC1_DATA, ICW4_8086);
	iowait();
	outb(PIC2_DATA, ICW4_8086);
	iowait();
	/* Mask everything */ /* except 2nd pin of master */
	outb(PIC1_DATA, 0x7F);
	iowait();
	outb(PIC2_DATA, 0xFF);
	iowait();
	/* Set Interrupts */
	__asm__ __volatile__("sti");
}

void pic_ack(uint8_t irq_id)
{
	if (irq_id > 8)
		outb(PIC2_CMD, PIC_EOI);
	outb(PIC1_CMD, PIC_EOI);
	
}
