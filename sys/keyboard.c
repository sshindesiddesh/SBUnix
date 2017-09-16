#include <sys/keyboard.h>
#include <sys/kprintf.h>

unsigned char s_code;

void pic_ack(uint8_t irq_id);

unsigned char inb(unsigned short port)
{
	unsigned char c;
	__asm__ __volatile__ ("inb %1, %0" : "=a" (c) : "dN" (port));
	return c;
}

static char key_map[] = {
	' ', ' ', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	' ', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', ' ', ' ', ' ', ' ',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', ' ', '*', ' ', ' ', ' ',
};

static char shift_key_map[] = {
	' ', ' ', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n',
	' ', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ' ', ' ', ' ', ' ',
	'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', ' ', '*', ' ', ' ', ' ',
};

uint8_t shift = 0, ctrl = 0, last_key = 0;

void update_key(int key, int ctrl);

void __isr_keyboard_cb()
{
#if 0
	s_code = inb(0x60);
	kprintf("%x\n", s_code);

	int a = key_map[0]; a++;
	int b = shift_key_map[0]; b++;
#endif

	s_code = inb(0x60);
	switch (s_code) {
		case 0x36 :
		case 0x2A :
			shift = 0x2A;
		break;
		case 0x1D :
			ctrl = 0x1D;
		break;
		default :
			if (s_code > 0x80) {
				if ((s_code - 0x80 == last_key) && shift == 0 && ctrl == 0)
					update_key(key_map[last_key], 0);
				else if (s_code - 0x80 == shift) {
					update_key(shift_key_map[last_key], 0);
					shift = 0;
				} else if (s_code - 0x80 == ctrl) {
					update_key(shift_key_map[last_key], 1);
					ctrl = 0;
				}
			} else {
				last_key = s_code;
			}
		break;
	}
	pic_ack(0x21);
}
