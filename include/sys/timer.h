#ifndef _TIMER_H
#define _TIMER_H

#include <sys/defs.h>
void timer_init();
void __isr_timer_cb(uint64_t count);
#endif
