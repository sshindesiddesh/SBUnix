#ifndef _TARFS_H
#define _TARFS_H
#include <sys/defs.h>
#include <sys/process.h>

#define FILE_TYPE 1
#define DIRECTORY 2
#define PAGE_TABLE_ALIGNLENT 0x1000
#define ALIGN_DOWN(x)   (x & ~(PAGE_TABLE_ALIGNLENT-1))

extern char _binary_tarfs_start;
extern char _binary_tarfs_end;

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

//TARFS file system entry
typedef struct {
    char name[100];
    int size;
    int type;
    uint64_t addr_header;
    int parent_index;
} tarfs_entry_t;

void tarfs_init();
uint64_t check_file_exists(char* filename);
pcb_t * create_elf_process(char *filename, char *argv[]);
vma_t *malloc_vma(mm_t *mm);
#endif
