#include <stdio.h>
#include "../lib/freeblist.h"

static int check_blist(int s)
{
	struct blist_t *b = blist_init(s);
	int i, idx;

	for (i = 0; i < s; i++) {
		idx = blist_get_nextz(b);
		if (idx != i) {
			printf("idx (%d) not %d\n", idx, i);
			return -1;
		} else
			blist_set(b, idx);
	}
	for (i = 0; i < s; i++) {
		if (!blist_isset(b, i)) {
			printf("%d not set\n", i);
			return -1;
		} else
			blist_reset(b, i);
	}
	for (i = 0; i < s; i++) {
		if (blist_isset(b, i)) {
			printf("%d is set\n", i);
			return -1;
		}
	}
	blist_free(b);
	return 0;
}

int main(int argc, char *argv[])
{
	int s = 1<<10;
	if (check_blist(s)) {
		printf("Failed for size: %d\n", s);
		return -1;
	}

	s = 1;
	if (check_blist(s)) {
		printf("Failed for size: %d\n", s);
		return -1;
	}
	return 0;
}
