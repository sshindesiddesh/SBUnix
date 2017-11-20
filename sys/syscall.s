#include <sys/config.h>
.text

.global __syscall_yield
__syscall_yield:
	mov $24, %rax
	int $0x80
	retq

.global __syscall_info
__syscall_info:
	mov $99, %rax
	int $0x80
	retq

.global __syscall_write
__syscall_write:
	mov $1, %rax
	int $0x80
	retq
