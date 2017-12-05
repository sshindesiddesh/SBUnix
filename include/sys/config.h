#ifndef _KERN_CONFIG_H
#define _KERN_CONFIG_H

#define ENABLE_TIMER		1
#define ENABLE_KEYBOARD		1
/* User paging should be disabled for using user processes code in kernel space
 * e.g. simple functions. */
#define ENABLE_USER_PAGING	1

#define PREEMPTIVE_SCHED	0
#if PREEMPTIVE_SCHED
#undef	ENABLE_TIMER
#define	ENABLE_TIMER		1
#endif

#endif
