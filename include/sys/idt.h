#ifndef _IDT_H
#define _IDT_H


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

void isr20(void);
void isr21(void);
WEAK void isr22(void);
WEAK void isr23(void);
WEAK void isr24(void);
WEAK void isr25(void);
WEAK void isr26(void);
WEAK void isr27(void);


WEAK void excp0(void);
WEAK void excp1(void);
WEAK void excp2(void);
WEAK void excp3(void);
WEAK void excp4(void);
WEAK void excp5(void);
WEAK void excp6(void);
WEAK void excp7(void);
WEAK void excp8(void);
WEAK void excp9(void);
WEAK void excpA(void);
WEAK void excpB(void);
WEAK void excpC(void);
WEAK void excpD(void);
WEAK void excpE(void);
WEAK void excpF(void);

WEAK void excp10(void);
WEAK void excp11(void);
WEAK void excp12(void);
WEAK void excp13(void);
WEAK void excp14(void);
WEAK void excp15(void);
WEAK void excp16(void);
WEAK void excp17(void);
WEAK void excp18(void);
WEAK void excp19(void);
WEAK void excp1A(void);
WEAK void excp1B(void);
WEAK void excp1C(void);
WEAK void excp1D(void);
WEAK void excp1E(void);
WEAK void excp1F(void);

void *memset(void *ptr, int value, size_t len);
#endif
