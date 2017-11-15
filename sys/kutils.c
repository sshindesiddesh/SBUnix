#include <sys/defs.h>
#include <sys/kutils.h>

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
