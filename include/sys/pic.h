#ifndef _PIC_H
#define _PIC_H
#include <sys/defs.h>

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

void outb(uint16_t port, uint8_t val);
unsigned char inb(unsigned short port);
void pic_ack(uint8_t irq_id);
void set_mask(uint8_t irq_id);
void clr_mask(uint8_t irq_id);
void pic_init();

#endif
