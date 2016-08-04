/*
 * Create #n worker threads and deposit work objects to them.
 * Create a dispatcher thread that:
 * a> deposits work to the workers.
 * b> Once a worker is done, go back to a.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "dispatcher.h"

/* Pull together all the dispatcher 2 worker items together */
struct msg_dispatcher_t {
	int				n_workers;
	struct worker2dispatcher_t	w2d;
	struct work_object_t		*workers;
	int				s_work_item;
	char				items[0];
};

struct worker2dispatcher_t *msg_dispatcher_get_w2d(
					struct msg_dispatcher_t *d)
{
	return &d->w2d;
}


static inline char *get_work_item_loc(char *s, int size, int index)
{
	return(s + (size * index));
}

struct msg_dispatcher_t *init_msg_dispatcher(int n_workers, int s_work_item)
{
	int i;
	struct msg_dispatcher_t *mds = malloc(sizeof(*mds) +
					      (s_work_item * n_workers));
	char *loc;

	mds->n_workers = n_workers;
	mds->s_work_item = s_work_item;
	/* init worker2dispatcher */
	init_worker2dispatcher(&mds->w2d, n_workers);
	mds->workers = malloc(sizeof(struct work_object_t)*n_workers);
	/* init worker objects */
	for (i = 0; i < n_workers; i++) {
		loc = get_work_item_loc(mds->items, s_work_item, i); 
		init_work_object(&mds->workers[i], i, loc);
	}
	return mds;
}

void init_parms_worker(struct msg_dispatcher_t *mds,
		       struct parms_worker_t *p_w, int n_workers,
		       int (*w_f)(void *p, char *c), void **wps)
{
	int i;
	for (i = 0; i < n_workers; i++) {
		p_w[i].wo = &mds->workers[i];
		p_w[i].w2d = &mds->w2d;
		p_w[i].work_func = w_f;
		p_w[i].wps = wps[i];
	}
}

void init_parms_dispatcher(struct parms_dispatcher_t *p_d, 
			   int n_jobs, struct msg_dispatcher_t *mds,
			   void *cfg_disp)
{
	p_d->num_jobs = n_jobs;
	p_d->d =  mds;
	p_d->cfg_disp = cfg_disp;
}

void *worker(void *p)
{
	struct parms_worker_t *w = p;
	pthread_t tid;
	struct timespec ts;
	// int rc;

	tid = pthread_self();
	while (1) {
		pthread_mutex_lock(&w->wo->m_work);
		// clock_gettime(CLOCK_REALTIME, &ts);
		// ts.tv_sec += 5;
		pthread_cond_wait(&w->wo->work_signal, &w->wo->m_work);
		pthread_mutex_unlock(&w->wo->m_work);
		/* do the work now */
		if (w->work_func(w->wps, w->wo->work_item) < 0)
			return;
		/* Tell dispatcher that we r done. */
		w2d_set_done(w->w2d, w->wo->worker_id);
	}
}

int dispatch_next(struct msg_dispatcher_t *d,
		  char *item)
{
	/* find an empty worker thread */
	int i;
	struct worker2dispatcher_t *w2d = &d->w2d;
	pthread_mutex_t m_tmp;
	char *loc;

	pthread_mutex_init(&m_tmp, NULL);
	do {
		for (i = 0; i < w2d->total_workers; i++) {
			if (w2d_is_done(w2d, i))
				break;
		}
		if (i == w2d->total_workers) {
			/* wait for a worker to be done. */
			pthread_mutex_lock(&m_tmp);
			pthread_cond_wait(&w2d->work_done, &m_tmp);
			pthread_mutex_unlock(&m_tmp);
			/* TBD: when non-blocking support is added
			return -ENOENT;
			*/
		}
	} while (i == w2d->total_workers);
	w2d_reset(w2d, i);
	pthread_mutex_lock(&d->workers[i].m_work);
	loc = get_work_item_loc(d->items, d->s_work_item, i);
	memcpy(loc, item, d->s_work_item);
	pthread_mutex_unlock(&d->workers[i].m_work);
	pthread_cond_signal(&d->workers[i].work_signal);
	return 0;
}

/* wait for all the workers to finish up. */
void wait_all_workers(struct msg_dispatcher_t *d)
{
	struct worker2dispatcher_t *w2d = &d->w2d;
	pthread_mutex_t m_tmp;
	int c = 0;

	pthread_mutex_init(&m_tmp, NULL);
	do {
		if (!w2d_is_done(w2d, c)) {
			pthread_mutex_lock(&m_tmp);
			pthread_cond_wait(&w2d->work_done, &m_tmp);
			pthread_mutex_unlock(&m_tmp);
			c = 0;
		} else
			c++;
	} while (c < w2d->total_workers);
}

void dispatch_all_workers(struct msg_dispatcher_t *d, char *item)
{
	int i;
	struct worker2dispatcher_t *w2d = &d->w2d;
	char *loc;
	for (i = 0; i < w2d->total_workers; i++) {
		pthread_mutex_lock(&d->workers[i].m_work);
		loc = get_work_item_loc(d->items, d->s_work_item, i);
		memcpy(loc, item, d->s_work_item);
		pthread_mutex_unlock(&d->workers[i].m_work);
		pthread_cond_signal(&d->workers[i].work_signal);
	}
}
