#ifndef _IDT_H
#define _IDT_H
#include <sys/process.h>

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

#define WEAK  __attribute__((weak))

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

void init_idt();
extern pcb_t *cur_pcb;
void kkill(uint64_t pid);

void isr20(void);
void isr21(void);
WEAK void isr80(void)
{
	kprintf("Exception 80\n");
	while (1);
}
WEAK void isr22(void)
{
	kprintf("Exception 22\n");
	while (1);
}
WEAK void isr23(void)
{
	kprintf("Exception 23\n");
	while (1);
}
WEAK void isr24(void)
{
	kprintf("Exception 24\n");
	while (1);
}
WEAK void isr25(void)
{
	kprintf("Exception 25\n");
	while (1);
}
WEAK void isr26(void)
{
	kprintf("Exception 26\n");
	while (1);
}
WEAK void isr27(void)
{
	kprintf("Exception 27\n");
	while (1);
}


WEAK void excp0(void)
{
	kprintf("\nUser Application Triggered Devide By Zero Exception\n");
	kprintf("\nKilled User Process...\n");
	kkill(cur_pcb->pid);
	//while (1);
}
WEAK void excp1(void)
{
	kprintf("Exception 1\n");
	while (1);
}
WEAK void excp2(void)
{
	kprintf("Exception 2\n");
	while (1);
}
WEAK void excp3(void)
{
	kprintf("Exception 3\n");
	while (1);
}
WEAK void excp4(void)
{
	kprintf("Exception 4\n");
	while (1);
}
WEAK void excp5(void)
{
	kprintf("Exception 5\n");
	while (1);
}
WEAK void excp6(void)
{
	kprintf("Exception 6\n");
	while (1);
}
WEAK void excp7(void)
{
	kprintf("Exception 7\n");
	while (1);
}
WEAK void excp8(void)
{
	kprintf("Exception 8\n");
	while (1);
}
WEAK void excp9(void)
{
	kprintf("Exception 9\n");
	while (1);
}
WEAK void excpA(void)
{
	kprintf("Exception A\n");
	while (1);
}
WEAK void excpB(void)
{
	kprintf("Exception B\n");
	while (1);
}
WEAK void excpC(void)
{
	kprintf("Exception C\n");
	while (1);
}

WEAK void excpD(void)
{
	kprintf("Exception D\n");
	while (1);
}

#if 0
void WEAK excpE(void)
{
	kprintf("Exception E\n");
	while (1);
}
#endif
void WEAK excpE(void);

WEAK void excpF(void)
{
	kprintf("Exception F\n");
	while (1);
}

WEAK void excp10(void)
{
	kprintf("Exception 10\n");
	while (1);
}
WEAK void excp11(void)
{
	kprintf("Exception 11\n");
	while (1);
}
WEAK void excp12(void)
{
	kprintf("Exception 12\n");
	while (1);
}
WEAK void excp13(void)
{
	kprintf("Exception 13\n");
	while (1);
}
WEAK void excp14(void)
{
	kprintf("Exception 14\n");
	while (1);
}
WEAK void excp15(void)
{
	kprintf("Exception 15\n");
	while (1);
}
WEAK void excp16(void)
{
	kprintf("Exception 16\n");
	while (1);
}
WEAK void excp17(void)
{
	kprintf("Exception 17\n");
	while (1);
}
WEAK void excp18(void)
{
	kprintf("Exception 18\n");
	while (1);
}
WEAK void excp19(void)
{
	kprintf("Exception 19\n");
	while (1);
}
WEAK void excp1A(void)
{
	kprintf("Exception 1A\n");
	while (1);
}
WEAK void excp1B(void)
{
	kprintf("Exception 1B\n");
	while (1);
}
WEAK void excp1C(void)
{
	kprintf("Exception 1C\n");
	while (1);
}
WEAK void excp1D(void)
{
	kprintf("Exception 1D\n");
	while (1);
}
WEAK void excp1E(void)
{
	kprintf("Exception 1E\n");
	while (1);
}
WEAK void excp1F(void)
{
	kprintf("Exception 1F\n");
	while (1);
}

void *memset(void *ptr, int value, size_t len);
#endif
