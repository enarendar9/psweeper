/*
 * Message format from/to dispather to workers
 */
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>

struct worker2dispatcher_t {
	pthread_cond_t work_done;   	/* from worker to dispatcher */
	int  total_workers;
	volatile int  *worker_done_list;
};

static inline w2d_is_done(struct worker2dispatcher_t *w2d, int i)
{
	volatile int *p;
	assert(w2d->total_workers > i);
	p = &w2d->worker_done_list[i];
	return *p == 1;
}

static inline w2d_set_done(struct worker2dispatcher_t *w2d, int i)
{
	volatile int *p;
	assert(w2d->total_workers > i);
	p = &w2d->worker_done_list[i];
	/* volatile reference...goes to memory */
	*p  = 1;
	pthread_cond_signal(&w2d->work_done);
}

static inline w2d_reset(struct worker2dispatcher_t *w2d, int i)
{
	volatile int *p = &w2d->worker_done_list[i];
	assert(w2d->total_workers > i);
	p = &w2d->worker_done_list[i];
	/* volatile reference...goes to memory */
	*p = 0;
}

static void init_worker2dispatcher(struct worker2dispatcher_t *w2d,
				   int total_workers)
{
	int i;
	volatile int *p;
	pthread_cond_init(&w2d->work_done, NULL);
	w2d->total_workers = total_workers;
	w2d->worker_done_list = malloc(sizeof(int) * total_workers);
	for (i = 0; i < w2d->total_workers; i++) {
		p = &w2d->worker_done_list[i];
		*p = 1;
	}
}

static inline struct worker2dispatcher_t *create_worker2dispatcher(int num_workers)
{
	struct worker2dispatcher_t *w2d = malloc(sizeof(*w2d));
	init_worker2dispatcher(w2d, num_workers);
	return w2d;
}

struct work_object_t {
	int		worker_id;  /* id of this worker */
	pthread_cond_t	work_signal; /* from dispatcher to workers */
	pthread_mutex_t	m_work;     /* Mutex around work item */
	void 		*work_item;
};

static inline init_work_object(struct work_object_t *wo, int id, void *w_i)
{
	wo->worker_id = id;
	pthread_cond_init(&wo->work_signal, NULL);
	pthread_mutex_init(&wo->m_work, NULL);
	wo->work_item = w_i;
}

/*
 * API's and structs for dispatcher/workers to be consumed
 */

struct msg_dispatcher_t;
/* Params for the dispatcher thread */
struct parms_dispatcher_t {
	int 				num_jobs;
	struct msg_dispatcher_t		*d;
	void				*cfg_disp; /* config for dispatcher
							thread */
};

/* Params for the worker thread(s) */
struct parms_worker_t {
	struct work_object_t		*wo;
	struct worker2dispatcher_t	*w2d;
	int (*work_func)(void *params, char *p_work);
	void 				*wps; /* other var's for the worker
						to use */
};

extern struct msg_dispatcher_t *init_msg_dispatcher(int n_workers,
						    int s_work_items);
extern struct worker2dispatcher_t *msg_dispatcher_get_w2d(
					struct msg_dispatcher_t *d);
extern void init_parms_worker(struct msg_dispatcher_t *mds,
			      struct parms_worker_t *p_w, int n_workers,
			      int (*w_f)(void *p, char *c), void **wps);
extern void init_parms_dispatcher(struct parms_dispatcher_t *p_d, 
				  int n_jobs, struct msg_dispatcher_t *mds,
			          void *cfg_disp);
extern void *worker(void *p);
extern int dispatch_next(struct msg_dispatcher_t *d, char *item);
extern void wait_all_workers(struct msg_dispatcher_t *d);
extern void dispatch_all_workers(struct msg_dispatcher_t *d, char *item);
