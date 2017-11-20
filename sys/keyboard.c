#include <sys/keyboard.h>
#include <sys/kprintf.h>
#include <sys/pic.h>

unsigned char s_code;

uint8_t shift = 0, ctrl = 0, last_key = 0;

void update_key(int key, int ctrl);

extern uint64_t reading_flag;
void __isr_keyboard_cb()
{
#if 0
	s_code = inb(0x60);
	kprintf("%x\n", s_code);

	int a = key_map[0]; a++;
	int b = shift_key_map[0]; b++;
#endif
	char key;
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
				if ((s_code - 0x80 == last_key) && shift == 0 && ctrl == 0) {
					key = key_map[last_key];
					update_key(key, 0);
					if (reading_flag)
						update_read_buf(key);
				} else if (s_code - 0x80 == shift) {
					key = shift_key_map[last_key];
					update_key(key, 0);
					if (reading_flag)
						update_read_buf(key);
					shift = 0;
				} else if (s_code - 0x80 == ctrl) {
					key = shift_key_map[last_key];
					update_key(key, 1);
					if (reading_flag)
						update_read_buf(key);
					ctrl = 0;
				}
			} else {
				last_key = s_code;
			}
		break;
	}
	pic_ack(0x21);
}
