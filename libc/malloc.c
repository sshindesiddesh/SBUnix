#include <sys/defs.h>
#include <unistd.h>

typedef long Align;
#define NALLOC 1024
union header {
	struct {
		union header *ptr;
		unsigned size;
	} s;
	Align x;
};

typedef union header Header;
static Header base;
static Header *freep = NULL;

int printf(const char *fmt, ...);
void free(void *ap)
{
	Header *bp, *p;

	bp = (Header *)ap - 1;
	for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
		if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
			break;

	if (bp + bp->s.size == p->s.ptr) {
		bp->s.size += p->s.ptr->s.size;
		bp->s.ptr = p->s.ptr->s.ptr;
	} else
		bp->s.ptr = p->s.ptr;
	if (p + p->s.size == bp) {
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	} else
		p->s.ptr = bp;
	freep = p;
	printf("freep: %p", freep);
}

static Header *morecore(unsigned nunits)
{
	char *cp;
	Header *up;

	if (nunits < NALLOC)
		nunits = NALLOC;
	printf(" calling brk");
	uint64_t npages = (nunits + sizeof(Header))/4096 + 1;
	uint64_t sbrk = brk(npages);
	if(sbrk == -1)
	{
		printf("No more memory");
		return NULL;
	} else
		printf("brk returnd, %p", sbrk);
	cp = (char *)sbrk;
	if (cp == (char *) - 1)
		return NULL;
	up = (Header *)cp;
	up->s.size = nunits;
	free((void *)(up + 1));
	printf(" freep:%p ", freep); 
	return freep;
}

void *malloc(size_t size)
{
	Header *p, *prevp;
	Header *morecore(unsigned);
	size_t nunits;

	nunits = (size + sizeof(Header)-1)/sizeof(Header) + 1;
	if ((prevp = freep) == NULL) {
		base.s.ptr = freep = prevp = &base;
		base.s.size = 0;
	}
	for (p = prevp->s.ptr; ; prevp=p, p = p->s.ptr) {
		if (p == freep)
			if ((p = morecore(nunits)) == NULL) {
				printf("morecore returned no memory");
				return NULL;
			}
		if (p->s.size >= nunits) {
			if (p->s.size == nunits)
				prevp->s.ptr = p->s.ptr;
			else {
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}
			freep = prevp;
			printf("address returned %p", (void *)(p + 1));
			return (void *)(p + 1);
		}
	}
}
