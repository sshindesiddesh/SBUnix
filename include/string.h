#ifndef _STRING_H
#define _STRING_H

#include <sys/defs.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
size_t strlen(const char *buf);
size_t strcmp(const char *s1, const char *s2);
size_t strncmp(const char *s1, const char *s2, int len);
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
char *strtok(char *str, const char *delim);
#endif
