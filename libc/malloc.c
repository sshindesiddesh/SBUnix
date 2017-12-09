#include <sys/defs.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int printf(const char *fmt, ...);

#define PG_SIZE 4096

static int used_mem;
static int block_count;
static char *proc_heap_start = 0;
static char *proc_heap_end = 0;

enum {FREE, USED};
enum {NEW_HEADER, NO_HEADER, REUSE_HEADER};

typedef struct header_t {
	int status;
	int size;
} header_t, *header_p;

/* Make a free memory block */
void make_header(char *address, int size)
{
	header_p head = (header_p)address;
	head->status = FREE;
	head->size = size;
}

void *allocate_new_block(int new_size)
{
	char *address;
	header_p p_block;
	uint64_t header_size;
	int64_t n_pages = 0;

	header_size = sizeof(header_t);
	n_pages = (new_size + header_size) / (PG_SIZE + 1) + 1;

	/* Get pages from the kernel */
	address = (char *)brk(n_pages);

	/* Store memory start address for future reference */
	if (proc_heap_end == 0 || proc_heap_start == 0) {
		proc_heap_start = address;
		block_count = 0;
		used_mem = 0;
	}

	/* Allocate a used block */
	proc_heap_end = (char*)((uint64_t)address + (uint64_t)(PG_SIZE * n_pages));
	p_block = (header_p)address;
	p_block->status = USED;
	p_block->size = new_size + header_size;
	block_count++;

	/* Create a free block for remaining memory */
	if (PG_SIZE*n_pages > new_size + header_size) {
		make_header(((char *)p_block + new_size + header_size), (PG_SIZE * n_pages - new_size - header_size));
	}

	used_mem += new_size;

	/* Return address after the header, which user can use */
	return ((void *)p_block + header_size);
}

void *malloc(size_t size)
{
	int new_size, st_flag, header_size, temp = 0;
	header_p p_block;

	/* Align input size to header size */
	new_size = ((((size - 1) >> 3) + 1) << 3);

	/* No memory has been allocated yet, allocate new */
	if (proc_heap_end == 0) {
		return allocate_new_block(new_size);
	} else {
		st_flag = NO_HEADER;
		p_block = (header_p)proc_heap_start;
		header_size = sizeof(header_t);

		/* Check for free block from heap start to heap end */
		while (proc_heap_end >= ((char *)p_block + new_size + header_size)) {
			if (p_block->status == FREE) {
				if (p_block->size >= (new_size + header_size)) {
					st_flag = REUSE_HEADER;
					break;
				}
			}
			p_block = (header_p) ((char *)p_block + p_block->size);
		}

		/* If free block was found */
		if (st_flag != NO_HEADER) {
			p_block->status = USED;
			if (st_flag == REUSE_HEADER) {
				if (p_block->size > new_size + header_size) {
					/* Create used memory block */
					temp = p_block->size;
					p_block->size = new_size + header_size;
					/* Create free memory block from remainig memory */
					make_header(((char *)p_block + new_size + header_size), (temp - new_size - header_size));
				}
				block_count++;
			}
			used_mem += new_size;
			return ((char *)p_block + header_size);
		}
		
		/* No block found matching input memmory request */
		return allocate_new_block(new_size);
	}
}

void free(void *p)
{
	header_p t_ptr = (header_p)p;
	t_ptr--;
	t_ptr->status = FREE;
	used_mem = used_mem - (t_ptr->size + sizeof(header_t));
	block_count--;
}
