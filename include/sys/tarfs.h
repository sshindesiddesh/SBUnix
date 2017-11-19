#ifndef _TARFS_H
#define _TARFS_H
#include <sys/defs.h>
#include <sys/elf64.h>

#define FILE_TYPE 0
#define DIRECTORY 5
#define PAGE_TABLE_ALIGNLENT 0x1000
#define ALIGN_DOWN(x)   (x & ~(PAGE_TABLE_ALIGNLENT-1))

extern char _binary_tarfs_start;
extern char _binary_tarfs_end;
enum { RD_ONLY = 0, WR_ONLY = 1, O_RDWR = 2 };

struct posix_header_ustar {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char typeflag[1];
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
  char pad[12];
};

/* TARFS file system entry */
typedef struct tarfs_entry_t tarfs_entry_t;
typedef struct fd_t fd_t;
typedef struct dirent_t dirent_t;
typedef struct dir_t dir_t;

struct tarfs_entry_t {
	char name[64];
	int type;
	uint64_t start;
	uint64_t end;
	uint64_t current;
	tarfs_entry_t *child[20];
	uint64_t inode_no;
};

struct fd_t {
	uint64_t perm;
	uint64_t inode_no;
	uint64_t current;
	tarfs_entry_t *node;
};

struct dirent_t {
	uint64_t inode_no;
	char name[64];
};

struct dir_t {
	tarfs_entry_t *node;
	dirent_t cur_dirent;
	int fd;
	uint64_t current;
	char buf[64];
};

tarfs_entry_t *root; /* this is the "/" node and root of the complete tarfs tree structure */
void tarfs_init();
uint64_t check_file_exists(char* filename);
int file_read(fd_t *fd, char *buf, uint64_t length);
fd_t *file_open(char *path, uint64_t mode);
dir_t *opendir(char *path);
int closedir(dir_t * dir);
tarfs_entry_t * create_tarfs_entry(char *name, uint64_t type, uint64_t start, uint64_t end, uint64_t inode_no, tarfs_entry_t *parent);
int is_proper_executable(Elf64_Ehdr* header);
void * get_posix_header(char* filename);

#endif
