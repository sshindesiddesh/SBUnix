#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

DIR *opendir_sys(const char *pathname, DIR *dir);
struct dirent *readdir_sys(DIR *dirp, struct dirent *dir);

DIR *opendir(const char *name)
{
	DIR *ret = (DIR *)malloc(sizeof(DIR));
	ret = opendir_sys(name, ret);
	return ret;
}

struct dirent *readdir(DIR *dirp)
{
	struct dirent *ret = (struct dirent *)malloc(sizeof(struct dirent));
	ret = readdir_sys(dirp, ret);
	return ret;
}

int closedir(DIR *dirp)
{
	free(dirp);
	return 0;
}

