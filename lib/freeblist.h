/*
 * manage a bitlist of n items.
 * find a free bitlist....
 */
struct blist_t;

extern struct blist_t *blist_init(int s);
extern void blist_free(struct blist_t *b);
/* set the bit i to 1 */
extern void blist_set(struct blist_t *b, int i);
/* set the bit i to 0 */
extern void blist_reset(struct blist_t *b, int i);
extern int blist_isset(struct blist_t *b, int i);
/* find the next bit which is zero in the bitlist */
extern int blist_get_nextz(struct blist_t *b);
