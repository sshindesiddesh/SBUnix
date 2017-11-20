#ifndef _CONSOLE_H
#define _CONSOLE_H

/* -80 to skip the last line which is reserved for Timer and keyboard. */

#define MAX_SCREEN_Y	24
#define MAX_SCREEN_X	80

int putchar(int c);
int puts(const char *str);
void update();
void update_key(int key, int ctrl);
void update_time(uint64_t time);
char *generic_conv(unsigned long n, int b);
int write_console(int c, int x, int y);
void change_console_ptr();
uint64_t console_read(int fd, char *buf, uint64_t count);
#endif
