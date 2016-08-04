/*
 *
 */
#include <stdlib.h>
#include "freeblist.h"

struct blist_t {
	int	max;	/* maximum size */
	int	z_cnt;
	int	o_cnt;
	int	z_idx;
	int	o_idx;
	int	s;
	char	data[0];
	/* blist will be here.. */
};

struct blist_t *blist_init(int s)
{
	int csize;
//	int m = 1<<1;
	struct blist_t *bl;
	int blist_s;
#if 0
	do {
		if (s <= m)
			break;
		m <<= 1;
	} while(1);
#endif

	blist_s = s/8;
	if (s%8)
		blist_s += 1;

	bl = calloc(1, sizeof(*bl)+blist_s);
	if (!bl)
		return NULL;
	bl->s = blist_s;
	bl->max = s;
	bl->z_idx = 0;
	bl->o_idx = -1;
	bl->z_cnt = s;
	bl->o_cnt = 0;
	return(bl);
}

void blist_free(struct blist_t *b)
{
	free(b);
}

static inline void get_by_n_bi(int i, int *by, int *bi)
{
	*by = i/8;
	*bi  = (i%8) & 0xFF;
}

static inline char *get_blist(struct blist_t *b)
{
	return ((char *)b+sizeof(*b));
}

/* set the bit i to 1 */
void blist_set(struct blist_t *b, int i)
{
	char *blist = get_blist(b);
	int byte;
	int bit;
	char *c;
	if (i >= b->max)
		return;

	get_by_n_bi(i, &byte, &bit);
	c = &blist[byte];
	if (*c & (1 << bit))
		return;
	*c |= (1 << bit);
	b->o_cnt++;
	b->z_cnt--;
//	printf("i: %d c:0x%x byte: %d bit: %d\n", i, *c, byte, bit);
}

/* set the bit i to 0 */
extern void blist_reset(struct blist_t *b, int i)
{
	char *blist = get_blist(b);
	int byte;
	int bit;
	char *c;
	if (i >= b->max)
		return;

	get_by_n_bi(i, &byte, &bit);
	c = &blist[byte];
	if (*c & (1 << bit)) {
		*c &= ~(1<<bit);
		b->o_cnt--;
		b->z_cnt++;
		return;
	}
	*c |= (1 << bit);
	b->o_cnt++;
}

int blist_isset(struct blist_t *b, int i)
{
	char *blist = get_blist(b);
	int byte;
	int bit;
	char *c;
	if (i >= b->max)
		return -1;

	get_by_n_bi(i, &byte, &bit);
	c = &blist[byte];
	if (*c & (1 << bit))
		return 1;
	return 0;
}

int blist_get_nextz(struct blist_t *b)
{
	char *blist = get_blist(b);
	int byte;
	int bit;
	char *c, j;
	int i;

	for (i = 0; i < b->max; i += 8) {
		get_by_n_bi(i, &byte, &bit);
		c = &blist[byte];
		if (*c == 0xFF)
			continue;
		for (j = 0; j < 8 && (i+j) < b->max; j++) {
			if ((*c & (1<<j)) == 0)
				return (i+j);
		}
	}
	return -1;
}
