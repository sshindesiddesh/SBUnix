#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>
#include <sys/pic.h>
#include <sys/idt.h>

void pic_ack(uint8_t irq_id)
{
	if (irq_id > 0x28)
		outb(PIC2_CMD, PIC_EOI);
	outb(PIC1_CMD, PIC_EOI);
}

void set_mask(uint8_t irq_id)
{
	uint16_t port;
	uint8_t value;

	if (irq_id > 0x28) {
		port = PIC1_DATA;
		irq_id -= 0x20;
	} else {
		port = PIC2_CMD;
		irq_id -= 0x20;
	}
	value = inb(port) | (1 << irq_id);
	outb(port, value);
}

void clr_mask(uint8_t irq_id)
{
	uint16_t port;
	uint8_t value;

	if (irq_id > 0x28) {
		port = PIC1_DATA;
		irq_id -= 0x20;
	} else {
		port = PIC2_DATA;
		irq_id -= 0x28;
	}
	value = inb(port) & ~(1 << irq_id);
	outb(port, value);
}

void pic_init()
{
	__asm__ __volatile__("cli");
	/* Initialize Master */
	outb(PIC1_CMD, ICW1_ICW4 | ICW1_INIT);

	/* Initialize Slave */
	outb(PIC2_CMD, ICW1_ICW4 | ICW1_INIT);

	/* Offset for Master is IRQ0 */
	outb(PIC1_DATA, IRQ0);

	/* Offset for Slave is IRQ8 */
	outb(PIC2_DATA, IRQ8);

	/* Inform Pic about wired cascade */
	outb(PIC1_DATA, 0x04);
	outb(PIC2_DATA, 0x02);

	/* Mask everything */
	/* except Timer Interrupt */
	/* except Keyboard Interrupt */
	outb(PIC1_DATA, 0xFC);
	outb(PIC2_DATA, 0xFF);
	/* Set Interrupts */
	__asm__ __volatile__("sti");
}

void outb(uint16_t port, uint8_t val)
{
	__asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char inb(unsigned short port)
{
	unsigned char c;
	__asm__ __volatile__ ("inb %1, %0" : "=a" (c) : "dN" (port));
	return c;
}
