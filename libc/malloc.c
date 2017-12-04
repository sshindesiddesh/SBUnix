#include <sys/defs.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int printf(const char *fmt, ...);

#define PG_SIZE 4096

static int used_mem;
static int block_count;
static char *mem_start_addr;
static char *proc_heap_end;

enum {FREE, USED};
enum {NEW_Header, NO_Header, REUSE_Header};

typedef struct header_t {
	int status;
	int size;
} Header_t, *Header_P;

void make_header(char *address, int size)
{
	Header_P head = (Header_P)address;
	head->status = FREE;
	head->size = size;
}

void *allocate_new_block(int new_size)
{
	char *address;
	Header_P p_block;
	uint64_t header_size, n_pages = 0;

	header_size = sizeof(Header_t);
	n_pages = (new_size + header_size) /(PG_SIZE + 1) + 1;
	address = (char*)brk((uint64_t)n_pages);

	if (proc_heap_end == 0) {
		mem_start_addr = address;
		block_count = 0;
		used_mem = 0;
	}

	proc_heap_end = (char*)((uint64_t)address + (uint64_t)(PG_SIZE * n_pages));
	p_block = (Header_P)address;
	p_block->status = USED;
	p_block->size = new_size + header_size;
	block_count++;

	if (PG_SIZE*n_pages > new_size + header_size) {
		make_header(((char *)p_block + new_size + header_size), (PG_SIZE * n_pages - new_size - header_size));
	}

	used_mem += new_size;
	return ((void *)p_block + header_size);
}

void *malloc(size_t size)
{
	Header_P p_block;
	int new_size, st_flag, header_size, temp = 0;

	/* Align input size to header size */
	new_size = ((((size - 1) >> 3) + 1) << 3);

	if (proc_heap_end == 0) {
		/* No memory has been allocated yet */
		return allocate_new_block(new_size);
	} else {
		st_flag = NO_Header;
		p_block = (Header_P)mem_start_addr;
		header_size = sizeof(Header_t);

		while (proc_heap_end >= ((char *)p_block + new_size + header_size)) {
			if (p_block->status == FREE) {
				if (p_block->size >= (new_size + header_size)) {
					st_flag = REUSE_Header;
					break;
				}
			}
			p_block = (Header_P) ((char *)p_block + p_block->size);
		}

		if (st_flag != NO_Header) {
			p_block->status = USED;
			if (st_flag == REUSE_Header) {
				if (p_block->size > new_size + header_size) {
					temp = p_block->size;
					p_block->size = new_size + header_size;
					make_header(((char *)p_block + new_size + header_size), (temp - new_size - header_size));
				}
				block_count++;
			}
			used_mem += new_size;
			return ((char *)p_block + header_size);
		}
		
		/* No block is found matching input mem request */
		return allocate_new_block(new_size);
	}
}

void free(void *p)
{
	Header_P t_ptr = (Header_P )p;
	t_ptr--;
	t_ptr->status = FREE;
	used_mem = used_mem - (t_ptr->size + sizeof(Header_t));
	block_count--;
}

