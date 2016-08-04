/*
 * collector.h: Collects results from the msgq and outputs to the given file.
 */
#ifndef __COLLECTOR_H__
#define __COLLECTOR_H__
struct parms_collector_t;
extern struct parms_collector_t *init_parms_collector(struct msgq_t **p_m,
						      int n_workers,
						      const char *name);
extern void collector_stop(struct parms_collector_t *p);
extern void *collector(void *p);
#endif
