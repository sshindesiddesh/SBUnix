#include <sys/defs.h>
#include <sys/kutils.h>

/* zeero out given string */
void zero_out(void *buf, int b)
{
	char *str = buf;
	while (b != 0) {
		*str++ = 0;
		b--;
	}
}

char *strcat(char *dst, char* src)
{
       size_t dst_len = strlen(dst);
       size_t i;

       for (i = 0; src[i] != '\0'; i++)
	   dst[dst_len + i] = src[i];
       dst[dst_len + i] = '\0';

       return dst;
}

char *mystrtok_r(char *bstr, const char *delim, char **save)
{
        char *temp = bstr;
        if(temp == NULL)
        {
                if(*save == NULL)
                {
                        return NULL;
                }
                temp = *save;
        }
        else
        {
                *save = NULL;
        }
// Find a non delimiter character
        while(*temp)
        {
                const char *temp2 = delim;
                while(*temp2)
                {
                        if(*temp == *temp2)
                        {
                                // This is a delimiter character
                                *temp = '\0';
                                break;
                        }
                        temp2++;
                }
                if(*temp != '\0')
                {
// Found the non delimiter character
                        break;
                }
                temp++;
        }
        if(*temp == '\0')
        {
                // No token found in remaining string.
                *save = NULL;
                return NULL;
        }
// Find a delimiter character
        char *retval = temp;
        while(*temp)
        {
                const char *temp2 = delim;
                while(*temp2)
                {
                        if(*temp == *temp2)
                        {
                                // This is a delimiter character
                                *save = temp + 1;
                                *temp = '\0';
                                break;
                        }
                        temp2++;
                }
if(*save > temp)
                {
                        // Found a delimiter character, stop searching for remaining occurrences
                        break;
                }
                temp++;
        }
        if(*save <= temp)
        {
                // Reached the end of original string, but no delimiter character found.
                // Mark save as NULL so that function can return NULL in next call, indicating the end of tokens in given string.
                *save = NULL;
        }

        return retval;
}
char *strtok(char *bstr, const char *delim)
{
        static char *save = NULL;
        return mystrtok_r(bstr, delim, &save);
}

size_t strncmp(const char *s1, const char *s2, int len)
{
        if (!s1 || !s2)
                return -1;

        size_t cnt = 0;

        while (len--)
                if (*s1++ != *s2++)
                        cnt++;
        return cnt;
}

char* strcpy(char *dst, const char *src)
{
	char *temp = dst;
	if(!src) {
		*dst = '\0';
		return dst;
	}
	while(*src) {
		*temp = *src;
		temp++;
		src++;
	}
	*temp = *src;
	return dst;
}

size_t strlen(const char *buf)
{
	size_t len = 0;
	while (*buf++ != '\0') len++;
	return len;
}

size_t strcmp(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;

	if (strlen(s1) != strlen(s2))
		return -1;

	size_t cnt = 0, len = strlen(s1);

	while (len--)
		if (*s1++ != *s2++)
			cnt++;
	return cnt;
}

long stoi(const char *s)
{
	long i;
	i = 0;
	while(*s >= '0' && *s <= '9') {
		i = i * 10 + (*s - '0');
		s++;
	}
	return i;
}

uint64_t power(uint64_t num, int e)
{
	if (e == 0) return 1;
	return num * power(num, e-1);
}

uint64_t octal_to_decimal(uint64_t octal)
{
	uint64_t dec = 0, i = 0;
	while(octal != 0) {
		dec = dec + (octal % 10) * power(8,i++);
		octal = octal/10;
	}
	return dec;
}
/* Memory Copy function */
void *memcpy(void *dest, const void *src, int n)
{
	char *d = (char *)dest;
	char *s = (char *)src;
	int i;
	for (i = 0; i < n; i++)
		d[i] = s[i];
	return d;
}

/* Function to Initialize memory */
void *memset(void *ptr, int value, size_t len)
{
	uint8_t *p = (uint8_t *)ptr;
	while (len--)
		*p++ = value;
	return ptr;
}

/* Memcopy function for 16 bit values */
unsigned short *memsetw(unsigned short *dest, unsigned short src, size_t count)
{
	unsigned short *t = (unsigned short *)dest;
	for( ; count != 0; count--) *t++ = src;
	return dest;
}
