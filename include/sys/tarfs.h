#ifndef _TARFS_H
#define _TARFS_H
#include <sys/defs.h>
#include <sys/elf64.h>
#include <sys/dirent.h>

#define O_RDONLY	0000		/* open for read only */
#define	O_WRONLY	0001		/* open for writing only */
#define	O_RDWR		0002		/* open for reading and writing */
#define FILE_TYPE 0
#define DIRECTORY 5
#define PAGE_TABLE_ALIGNLENT 0x1000
#define ALIGN_DOWN(x)   (x & ~(PAGE_TABLE_ALIGNLENT-1))

extern char _binary_tarfs_start;
extern char _binary_tarfs_end;
//enum { O_RDONLY = 0, O_WRONLY = 1, O_RDWR = 2 };

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
#if 0
/* file system entry */
typedef struct file_entry_t file_entry_t;
typedef struct fd_t fd_t;
typedef struct dirent dirent;
typedef struct dir_t dir_t;

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
#endif

file_entry_t *root; /* this is the "/" node and root of the complete tarfs tree structure */
void tarfs_init();
uint64_t check_file_exists(char* filename);
int tarfs_read(uint64_t fd_cnt, void *buf, uint64_t length);
int tarfs_open(char *path, uint64_t mode);
dir_t *tarfs_opendir_user(char *path, dir_t *retdir);
dir_t *tarfs_opendir(char *path);
int tarfs_closedir(dir_t * dir);
int tarfs_closedir_user(dir_t * dir);
file_entry_t * create_tarfs_entry(char *name, uint64_t type, uint64_t start, uint64_t end, uint64_t inode_no, file_entry_t *parent);
int is_proper_executable(Elf64_Ehdr* header);
void * get_posix_header(char* filename);
uint64_t tarfs_chdir(char *path);
int tarfs_close(int fd_c);
struct dirent *tarfs_readdir_user(uint64_t *dir, struct dirent *retdir);
struct dirent *tarfs_readdir(uint64_t *dir);
char *tarfs_getcwd(char *buf, size_t size);
uint64_t tarfs_listdir(char * buf, dir_t * dir);
int64_t tarfs_getdents(uint64_t fd, uint64_t buf, uint64_t count);
char *get_node_path(file_entry_t *p_node);

#endif
