#include <sys/keyboard.h>
#include <sys/kprintf.h>
#include <sys/pic.h>

unsigned char s_code;

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
					update_key(key_map[last_key], 1);
					ctrl = 0;
				}
			} else {
				last_key = s_code;
			}
		break;
	}
	pic_ack(0x21);
}
