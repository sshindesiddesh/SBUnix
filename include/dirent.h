#ifndef _DIRENT_H
#define _DIRENT_H

#define NAME_MAX 255
struct dirent {
	char           d_name[NAME_MAX+1];  /* Filename (null-terminated) */
};

/* file system entry */
typedef struct file_entry_t file_entry_t;
typedef struct fd_t fd_t;
typedef struct dirent dirent;
typedef struct dir_t dir_t;
typedef struct dir_t DIR;

struct file_entry_t {
	char name[64];
	int type;
	uint64_t start;
	uint64_t end;
	uint64_t current;
	file_entry_t *child[20];
	uint64_t inode_no;
};

struct fd_t {
	uint64_t perm;
	uint64_t inode_no;
	uint64_t current;
	file_entry_t *node;
};

struct dir_t {
	file_entry_t *node;
	dirent cur_dirent;
	int fd;
	uint64_t current;
	char buf[64];
};

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#endif
