#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/defs.h>
#include <sys/memory.h>
#include <sys/elf64.h>
#include <sys/console.h>
#include <sys/idt.h>
#include <sys/kutils.h>
#include <sys/process.h>

extern pcb_t *cur_pcb;

char extra_buf[100];

/* return the current working dir */
char *tarfs_getcwd(char *buf, size_t size)
{
	if (size < strlen(cur_pcb->current_dir))
		return NULL;
	strcpy(buf, cur_pcb->current_dir);
	
	return buf;
}

/* check whether input executable is proper by checking 1-3 MAgic numbers in ELF header, returns 0 on success */
int is_proper_executable(Elf64_Ehdr* header)
{
	if (header == NULL)
		return -1;
	else {
		if (header->e_ident[1] == 'E' && header->e_ident[2] == 'L' && header->e_ident[3] == 'F')
		{
#ifdef TARFS_DEBUG
			kprintf("\t executable verified");
#endif
			return 0;
		}
	}
	return -1;
}

/* check whether given file exists and return its posix header offset in tarfs */
void * get_posix_header(char* filename)
{
	file_entry_t *curr_node, *temp_node;
	char *name;
	int i = 0;
	curr_node = root;

	char temp_path[100] = "\0";
	strcpy(temp_path, filename);

	name = strtok(temp_path, "/");
	if (name == NULL)
		return NULL;

	while (name != NULL) {
		temp_node = curr_node;
		for (i = 2; i < curr_node->end; i++) {
			if (strcmp(name, curr_node->child[i]->name) == 0) {
				curr_node = (file_entry_t *)curr_node->child[i];
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
int tarfs_read(uint64_t fd_cnt, void *buf, uint64_t length)
{
	if ((cur_pcb->fd[fd_cnt] != NULL) && (cur_pcb->fd[fd_cnt]->perm != O_WRONLY)) {
		if (length > ((cur_pcb->fd[fd_cnt]->node->end) - (cur_pcb->fd[fd_cnt]->current)))
			length = (cur_pcb->fd[fd_cnt]->node->end) - (cur_pcb->fd[fd_cnt]->current);

		memcpy((void *)buf, (void *)(cur_pcb->fd[fd_cnt]->current), length);
		cur_pcb->fd[fd_cnt]->current += length;
#ifdef TARFS_DEBUG
		kprintf("\nbuffer read: %s", buf);
#endif
		return length;
	}
	else
		return -1;
}

/* open a file or directory, returns file/directory descriptor number on success, -1 on failure */
int tarfs_open(char *path, uint64_t mode)
{
	file_entry_t *node, *temp_node;
	char *name;
	char temp_path[100];
	int i = 0, fd_cnt = 2;
	fd_t *ret_fd = (fd_t *)kmalloc(sizeof(fd_t));
	node = root;

	strcpy(temp_path, path);

	if (strcmp(temp_path, "/") == 0) { /* special handling for only "/" */
		ret_fd->node = node;
		ret_fd->perm = mode;
		ret_fd->current = node->start;
		while ((cur_pcb->fd[++fd_cnt] != NULL) && fd_cnt < MAX_FD_CNT);
		if (fd_cnt >= MAX_FD_CNT) {
			return -1;
		}
		else {
			cur_pcb->fd[fd_cnt] = ret_fd;
			return fd_cnt;
		}
	}

	/* handle relative path */
	if (temp_path[0] != '/' && (strlen(temp_path) > 1))
		node = cur_pcb->current_node;

	name = strtok(temp_path, "/");
	if (name == NULL)
		return -1;

	/* handle . and .. in the path */
	if ((strcmp(name, "..") == 0) || (strcmp(name, ".") == 0))
		node = cur_pcb->current_node;
	while (name != NULL) {
		temp_node = node;
		if (strcmp(name, ".") == 0)
			node = node->child[0];
		else if (strcmp(name, "..") == 0)
			node = node->child[1];
		else {
			for (i = 2; i < node->end; i++) {
				if (strcmp(name, node->child[i]->name) == 0) {
					node = (file_entry_t *)node->child[i];
					break;
				}
			}
		}
		if (i >= temp_node->end)
		{
			return -1;
		}
		name = strtok(NULL, "/");
	}
	if (((node->type == DIRECTORY)/* && (mode == O_RDONLY)*/) || (node->type == FILE_TYPE)) {
		ret_fd->node = node;
		ret_fd->perm = mode;
		ret_fd->current = node->start;
	} else {
		return -1;
	}
	while ((cur_pcb->fd[++fd_cnt] != NULL) && fd_cnt < MAX_FD_CNT);
	if (fd_cnt >= MAX_FD_CNT)
	{
		return -1;
	}
	else {
		cur_pcb->fd[fd_cnt] = ret_fd;
		//kprintf(" fd returned %d ", fd_cnt);
		return fd_cnt;
	}
}

int tarfs_close(int fd_c)
{
	/* close all open file descriptor for current pcb */
	/* TODO free allocations doen previously */
	if (cur_pcb->fd[fd_c]) {
		cur_pcb->fd[fd_c] = NULL;
	}
	return 0;
}

struct dirent *tarfs_readdir_user(uint64_t *dir, struct dirent *retdir)
{
	struct dirent *l_dirent = tarfs_readdir(dir);
	if(l_dirent) {
		memcpy(retdir, l_dirent, sizeof(struct dirent));
		return retdir;
	} else
		return NULL;
}

/* read dirents from given directory */
struct dirent *tarfs_readdir(uint64_t *dir1)
{
	dir_t *dir = (dir_t *)dir1;
	if (dir->node->end < 3 || dir->current == dir->node->end || dir->current == 0)
		return NULL;
	else {
		strcpy(dir->cur_dirent.d_name, dir->node->child[dir->current]->name);
		dir->current += 1;
		return &dir->cur_dirent;
	}
}

/* getdents from given directory using fd */
int64_t tarfs_getdents(uint64_t fd, uint64_t buf, uint64_t count)
{
	if (cur_pcb->fd[fd] != NULL) {
		dirent *dir;
		dir = (dirent *)buf;
		strcpy(dir->d_name, cur_pcb->fd[fd]->node->child[(cur_pcb->fd[fd]->current)+2]->name);
		cur_pcb->fd[fd]->current += 1;
		return (uint64_t)dir;
	}
	else
		return (uint64_t)NULL;
}

/* read all the dirents from given directory and send out /t separated list */
uint64_t tarfs_listdir(char * buf, dir_t * dir)
{
	int count = 0;
	buf = "\0";
	if (dir) {
		dirent *dentry;
		while((dentry = tarfs_readdir((uint64_t *)dir)) != NULL) {
			count++;
			strcat(buf, dentry->d_name);
			strcat(buf, "\t");
		}
	}
	else
		kprintf(" open failed");

	return count;
}

int tarfs_closedir_user(dir_t * dir)
{
	return tarfs_closedir(dir);
}

/* close a directory */
int tarfs_closedir(dir_t * dir)
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
dir_t *tarfs_opendir_user(char *path, dir_t *ret_dir)
{
	dir_t *l_dir = tarfs_opendir(path);
	if(l_dir) {
		memcpy(ret_dir, l_dir, sizeof(dir_t));
		return ret_dir;
	} else
		return NULL;
}

/* open a directory, returns a descriptor to dir */
dir_t *tarfs_opendir(char *path)
{
	file_entry_t *node, *temp_node;
	char *name, *temp_path;
	int i = 0;
	dir_t * ret_dir;
	node = root;

	temp_path = (char *)kmalloc(64);
	strcpy(temp_path, path);

	if (strcmp(temp_path, "/") == 0) { /* special handling for only "/" */
		ret_dir = (dir_t *)kmalloc(sizeof(dir_t));
		ret_dir->current = 2;
		ret_dir->node = root;
#ifdef TARFS_DEBUG
		kprintf(" return :%s ", ret_dir->node->name);
#endif
		return ret_dir;
	}

	/* handle relative path */
	if (temp_path[0] != '/' && temp_path[0] != '.' && (strlen(temp_path) > 1))
		node = cur_pcb->current_node;

	name = strtok(temp_path, "/");
	if (name == NULL)
		return NULL;

	/* handle . and .. in the path */
	if ((strcmp(name, "..") == 0) || (strcmp(name, ".") == 0)) {
		node = cur_pcb->current_node;
	}

	while (name != NULL) {
		temp_node = node;
		if (strcmp(name, ".") == 0)
			node = node->child[0];
		else if (strcmp(name, "..") == 0)
			node = node->child[1];
		else {
			for (i = 2; i < node->end; i++) {
				if (strcmp(name, node->child[i]->name) == 0) {
					node = node->child[i];
					break;
				}
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

/* file traversal to find path */
char *get_node_path(file_entry_t *p_node)
{
	int node_i = 0, buf_i = 0;
	char temp_buf[20][20];
	zero_out(temp_buf, 400);
	extra_buf[0] = '\0';
	extra_buf[1] = '\0';
	char *s = extra_buf;
	file_entry_t *node = p_node;
	if (node == NULL) {
		return (char *)NULL;
	}
	if (node == root) {
		return (char *)node->name;
	}
	while (node != root) {
		strcpy(temp_buf[node_i],node->name);
		node_i++;
			node = node->child[1];
	}

	while (node_i >= 0) {
		while(temp_buf[node_i][buf_i] != '\0') {
			*s = temp_buf[node_i][buf_i];
			buf_i++;
			s++;
		}
		node_i--;
		buf_i = 0;
		if (node_i >= 0)
			*s++ = '/';
	}
	s++;
	*s = '\0';
	return extra_buf;
}

/* change the current working directory return 0 on success, -1 on failure */
uint64_t tarfs_chdir(char * path)
{
	char temp_path[100] = "\0";
	strcpy(temp_path, path);
	dir_t *dir = tarfs_opendir(temp_path);
	
	if ((dir == NULL) || (dir->node == NULL ))
		return -1;

	strcpy(cur_pcb->current_dir, get_node_path(dir->node));
	
	/* handle change of current node as well */
	cur_pcb->current_node = dir->node;
	return 0;
}

/* create node for given Dir or file in tarfs tree structure, return tarfs_entry */
file_entry_t * create_tarfs_entry(char *name, uint64_t type, uint64_t start, uint64_t end, uint64_t inode_no, file_entry_t *parent)
{
#ifdef TARFS_DEBUG
	kprintf(" new_entry: %s %d %p %p %d %s ", name, type, start, end, inode_no, parent->name);
#endif
	file_entry_t *entry = (file_entry_t *)kmalloc(sizeof(file_entry_t));
	strcpy(entry->name, name);
	entry->type = type;
	entry->start = start;
	entry->end = end;
	entry->inode_no = inode_no;
	entry->child[0] = entry;
	entry->child[1] = parent;
	entry->current = start;
	
	return entry;
}

/* parse individual entry(file or dir) found in tarfs */
void parse_tarfs_entry(char *path, int type, uint64_t start, uint64_t end)
{
	file_entry_t *temp1, *temp2, *temp3;
	char *name;
	int i = 0;

	char temp_path[100] = "\0";
	strcpy(temp_path, path);

	temp1 = root->child[2];
	name = strtok(temp_path, "/");
	if (name == NULL)
		return;

	while (name != NULL) {
		temp2 = temp1;
		for (i = 2; i < temp1->end; i++) {
			if (strcmp(name, temp1->child[i]->name) == 0) {
				temp1 = (file_entry_t *)temp1->child[i];
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
	file_entry_t *entry;

	/* allocate and place first node "/" */
	root = (file_entry_t *)kmalloc(sizeof(file_entry_t));
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
		size = 0;
		size = octal_to_decimal(stoi(tarfs_itr->size));
#ifdef TARFS_DEBUG
		kprintf(" entry: %s, size %d ", tarfs_itr->name, size);
#endif
		if (strcmp(tarfs_itr->typeflag, "5") == 0) /* directory entry */
			parse_tarfs_entry(tarfs_itr->name, DIRECTORY, 0, 2);
		else
			parse_tarfs_entry(tarfs_itr->name, FILE_TYPE, (uint64_t)(tarfs_itr + 1), (uint64_t)((void *)tarfs_itr + size + sizeof(struct posix_header_ustar)));
		if(size % 512 != 0) {
			size = (size/512)*512;
			size += 512;
		}
		tarfs_itr = (struct posix_header_ustar *)((uint64_t)tarfs_itr + size + sizeof(struct posix_header_ustar));
		end = (uint32_t *)tarfs_itr;
	}

}
