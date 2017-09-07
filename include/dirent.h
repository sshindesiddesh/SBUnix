#ifndef _DIRENT_H
#define _DIRENT_H

struct dirent {
	long  d_ino;     /* Inode number */
	off_t d_off;     /* Offset to next linux_dirent */
	unsigned short d_reclen;  /* Length of this linux_dirent */
	char           d_name[];  /* Filename (null-terminated) */
};
#endif
