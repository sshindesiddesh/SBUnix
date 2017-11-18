#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/defs.h>
#include <sys/memory.h>
#include <sys/elf64.h>
#include <sys/process.h>
#include <sys/console.h>
#include <sys/idt.h>
#include <sys/kutils.h>
//#define TARFS_DEBUG 1
#define O_RDONLY 1

/* check whether input executable is proper by checking 1-3 MAgic numbers in ELF header, returns 0 on success */
int is_proper_executable(Elf64_Ehdr* header)
{
	if (header == NULL)
		return -1;
	else {
		if (header->e_ident[1] == 'E' && header->e_ident[2] == 'L' && header->e_ident[3] == 'F')
		{
//#ifdef TARFS_DEBUG
			kprintf("\t executable verified");
//#endif
			return 0;
		}
	}
	return -1;
}

/* check whether given file exists and return its posix header offset in tarfs */
void * get_posix_header(char* filename)
{
	tarfs_entry_t *curr_node, *temp_node;
        char *name, *temp_path;
        int i = 0;
        curr_node = root;

        temp_path = (char *)kmalloc(64);
        strcpy(temp_path, filename);

        name = strtok(temp_path, "/");
        if (name == NULL)
                return NULL;

	while (name != NULL) {
		temp_node = curr_node;
		for (i = 2; i < curr_node->end; i++) {
			if (strcmp(name, curr_node->child[i]->name) == 0) {
				curr_node = (tarfs_entry_t *)curr_node->child[i];
				break;
			}
		}
		if (i == temp_node->end)
			return NULL;
		name = strtok(NULL, "/");
	}
	if (curr_node->type == FILE_TYPE)
		return (void *)curr_node->start;
	else
		return NULL;
}

/* read from a file into input buffer, returns number of bytes read */
int file_read(fd_t *fd, char *buf, uint64_t length)
{
	if (fd) {
		if (length > ((fd->node->end) - (fd->node->start)))
			length = fd->node->end - fd->node->start;
		memcpy((char *)buf, (char *)(fd->node->start), length);
#ifdef TARFS_DEBUG
		kprintf("\nbuffer read: %s", buf);
#endif
		return length;
	}
	else
		return -1;
}

/* open a file or directory, returns file/directory descriptor fd_t */
fd_t *file_open(char *path, uint64_t mode)
{
	tarfs_entry_t *node, *temp_node;
	char *name, *temp_path;
	int i = 0;
	fd_t *ret_fd = (fd_t *)kmalloc(sizeof(fd_t));
	node = root;

	temp_path = (char *)kmalloc(64);
	strcpy(temp_path, path);

	name = strtok(temp_path, "/");
	if (name == NULL)
		return NULL;

	if (strcmp(name, "rootfs") == 0) {
		while (name != NULL) {
			temp_node = node;
			for (i = 2; i < node->end; i++) {
				if (strcmp(name, node->child[i]->name) == 0) {
					node = (tarfs_entry_t *)node->child[i];
					break;
				}
			}
		if (i >= temp_node->end)
			return NULL;
		name = strtok(NULL, "/");
		}
		if ((node->type == DIRECTORY && mode == (O_RDONLY)) || (node->type == FILE_TYPE)) {
			ret_fd->node = node;
                        ret_fd->perm = mode;
                        return ret_fd;
                } else {
                        return NULL;
                }
	}
	else
		return NULL;
}

/* close a directory */
int closedir(dir_t * dir)
{
	if ((dir->node->type == DIRECTORY) && (dir->current > 1)) {
		dir->node = NULL;
		dir->current = 0;
		return 0;
	}
	else
		return -1;
}
	
/* open a directory, returns a descriptor to dir */
dir_t *opendir(char *path)
{
        tarfs_entry_t *node, *temp_node;
        char *name, *temp_path;
        int i = 0;
	dir_t * ret_dir;
        node = root;

        temp_path = (char *)kmalloc(64);
        strcpy(temp_path, path);

        name = strtok(temp_path, "/");
        if (name == NULL)
                return NULL;

        while (name !=NULL) {
		temp_node = node;
		for (i = 2; i < node->end; i++) {
			if (strcmp(name, node->child[i]->name) == 0) {
				node = node->child[i];
				break;
			}
		}
		if (i == temp_node->end)
			return NULL;
		name = strtok(NULL, "/");
	}
	if (node->type == DIRECTORY) {
		ret_dir = (dir_t *)kmalloc(sizeof(dir_t));
		ret_dir->current = 2;
                ret_dir->node = node;
#ifdef TARFS_DEBUG
		kprintf(" return :%s ", ret_dir->node->name);
#endif
		return ret_dir;
	}
	else
		return NULL;
}

/* create node for given Dir or file in tarfs tree structure, return tarfs_entry */
tarfs_entry_t * create_tarfs_entry(char *name, uint64_t type, uint64_t start, uint64_t end, uint64_t inode_no, tarfs_entry_t *parent)
{
#ifdef TARFS_DEBUG
	kprintf(" new_entry: %s %d %p %p %d %s ", name, type, start, end, inode_no, parent->name);
#endif
	tarfs_entry_t *entry = (tarfs_entry_t *)kmalloc(sizeof(tarfs_entry_t));
	strcpy(entry->name, name);
	entry->type = type;
	entry->start = start;
	entry->end = end;
	entry->inode_no = inode_no;
	entry->child[0] = entry;
	entry->child[1] = parent;
	
	return entry;
}

/* parse individual entry(file or dir) found in tarfs */
void parse_tarfs_entry(char *path, int type, uint64_t start, uint64_t end)
{
	tarfs_entry_t *temp1, *temp2, *temp3;
	char *name, *temp_path;
	int i = 0;

	temp_path = (char *)kmalloc(64);
        strcpy(temp_path, path);

	temp1 = root->child[2];
	name = strtok(temp_path, "/");
        if (name == NULL)
		return;

	while (name != NULL) {
		temp2 = temp1;
		for (i = 2; i < temp1->end; i++) {
			if (strcmp(name, temp1->child[i]->name) == 0) {
				temp1 = (tarfs_entry_t *)temp1->child[i];
				break;
			}
		}
		if (i == temp2->end) {
			temp3 = create_tarfs_entry(name, type, start, end, 0, temp2);
#ifdef TARFS_DEBUG
			kprintf(" p :%s c:%s ", temp2->name, temp3->name);
#endif
			temp1->child[temp1->end] = temp3;
			temp1->end += 1;
		}
		name = strtok(NULL, "/");
	}
}

void tarfs_init()
{
	tarfs_entry_t *entry;

	/* allocate and place first node "/" */
	root = (tarfs_entry_t *)kmalloc(sizeof(tarfs_entry_t));
	strcpy(root->name,"/");
	root->type = DIRECTORY;
	root->start = 0;
	root->end = 2;
	root->current = 0;
	root->child[0] = root;
	root->child[1] = root;
	root->inode_no = 0;
	
	/* allocate and place rootfs as child of "/" */
	entry = create_tarfs_entry("rootfs", DIRECTORY, 0, 2, 0, root);
	root->end += 1;
	root->child[2] = entry;

	/* TARFS starts at _binary_tarfs_start (use its address (&) to find the start of tarfs) */
	uint64_t size;
	struct posix_header_ustar *tarfs_itr = (struct posix_header_ustar *)&_binary_tarfs_start;
	uint32_t *end = (uint32_t *)tarfs_itr;

	/* parse till end is reached */
	while(*end++ || *end++ || *end) {
#ifdef TARFS_DEBUG
		kprintf(" entry: %s", tarfs_itr->name);
#endif
		size = 0;
		size = octal_to_decimal(stoi(tarfs_itr->size));
		if(size % 512 != 0) {
			size = (size/512)*512;
			size += 512;
		}
		if (strcmp(tarfs_itr->typeflag, "5") == 0) /* */
			parse_tarfs_entry(tarfs_itr->name, DIRECTORY, 0, 2);
		else
			parse_tarfs_entry(tarfs_itr->name, FILE_TYPE, (uint64_t)(tarfs_itr + 1), (uint64_t)((void *)tarfs_itr + size + sizeof(struct posix_header_ustar)));
		tarfs_itr = (struct posix_header_ustar *)((uint64_t)tarfs_itr + size + sizeof(struct posix_header_ustar));
		end = (uint32_t *)tarfs_itr;
	}
	struct posix_header_ustar *header = (struct posix_header_ustar *)get_posix_header("/rootfs/bin/sbush");
	Elf64_Ehdr *elf_header = (Elf64_Ehdr *)header;
	if (is_proper_executable(elf_header) == 0)
		kprintf(" done");
#ifdef TARFS_DEBUG
	kprintf("opendir /rootfs/bin :");
	dir_t * new = opendir("/rootfs/bin");
	if (new)
		kprintf("opendir success ");
	fd_t * new_fd = file_open("/rootfs/bin/abc.txt", 1);
	if(new_fd)
		kprintf(" file open success fd : %p ", new_fd);
	char *buf = (char *)kmalloc(1024);
	int a = file_read(new_fd, buf, 27);
	if (a != -1)
		kprintf("read content: %s ", buf);
	if (closedir(new) == 0)
		kprintf("dir closed");
#endif
}
