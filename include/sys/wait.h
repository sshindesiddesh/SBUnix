#ifndef _WAIT_H
#define _WAIT_H

#include <sys/defs.h>
pid_t waitpid(pid_t pid, int *status, int options);
#endif
