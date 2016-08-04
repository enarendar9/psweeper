/*
 * Create #n threads that generate message and post it to a 
 * collector.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define MAX_MSG 255
#define MSG_QUEUE 100
#define MAX_NAME 255
#define MAX_GEN 10

int n_gen = MAX_GEN;


struct coll_msg_t {
	char msg[MSG_QUEUE][MAX_MSG];
	int  w_index;
	int  r_index;
	int  items;
	pthread_mutex_t m_index;
};

/*
 * Generator to collector structures.
 */
pthread_cond_t signal_collector;
struct coll_msg_t mqueues[MAX_GEN];

/* Check if the message queue is empty?
 * Mutex is assumed to be held
 */
static inline int msgq_is_empty(const struct coll_msg_t *mq)
{
	return (mq->items == 0);
}

static inline int msgq_is_full(const struct coll_msg_t *mq)
{
	return (mq->items == MSG_QUEUE);
}

static inline int msgq_write(struct coll_msg_t *mq, pthread_cond_t *cond,
			     const char *msg)
{
	int r = 0;
	pthread_mutex_lock(&mq->m_index);
	if (msgq_is_full(mq)) {
		pthread_mutex_unlock(&mq->m_index);
		return -ENOTEMPTY;
	}
	strncpy(mq->msg[mq->w_index], msg, sizeof(mq->msg[mq->w_index]));
	mq->w_index = (mq->w_index + 1) % MSG_QUEUE;
	mq->items++;
	pthread_mutex_unlock(&mq->m_index);
	pthread_cond_broadcast(cond);
	return r;
}

/*
 * return a message char string.
 * caller must free it.
 */
static inline char *msgq_read(struct coll_msg_t *mq)
{
	size_t mlen = sizeof(mq->msg[0]);
	char *rstr = malloc(mlen);
	if (rstr == NULL)
		return NULL;
	pthread_mutex_lock(&mq->m_index);
	if (msgq_is_empty(mq)) {
		free(rstr);
		pthread_mutex_unlock(&mq->m_index);
		return NULL;
	}
	strncpy(rstr, mq->msg[mq->r_index], mlen);
	mq->r_index = (mq->r_index + 1) % MSG_QUEUE;
	mq->items--;
	pthread_mutex_unlock(&mq->m_index);
	return rstr;
}

/*
 * Collect messages from the generators
 * send them to the output.
 */
struct parms_collector_t {
	char fname[MAX_NAME];
	pthread_cond_t *signal;
	int  n_queues;
	struct coll_msg_t *p_msg_queues;
};
static void *collector(void *arg)
{
	struct parms_collector_t *p = arg;
	char *fname = p->fname;
	FILE *fp;
	pthread_mutex_t m_tmp;
	int i;
	char *msg;

	if (strcmp(p->fname, "STDOUT") == 0)
		fp = stdout;
	else
		fp = fopen(p->fname, "w");
	if (fp == NULL)
		return;

	pthread_mutex_init(&m_tmp, NULL);
	while (1) {
		pthread_mutex_lock(&m_tmp);
		pthread_cond_wait(p->signal, &m_tmp);
		for (i = 0; i < p->n_queues; i++) {
			if ((msg = msgq_read(&p->p_msg_queues[i]))) {
				fprintf(fp, "%d: %s\n", i, msg);
				free(msg);
			}
		}
		pthread_mutex_unlock(&m_tmp);
		fflush(fp);
	}
	/* TBD catch signal to shutdown the fp */
	fclose(fp);
	return NULL;
}

/* Generator(s) */
struct parms_generator_t {
	pthread_cond_t *signal; /* to notify the collector */
	struct coll_msg_t *p_msg_q; /* Generator's local queue */
};
static void *generator(void *arg)
{
	struct parms_generator_t *p = arg;
	int r, i;
	int count = 100;
	char msg[MAX_MSG];
	pthread_t tid;

	tid = pthread_self();
	for (i = 0; i < count; i++) {
		r = random() % 10;
		sleep(r);
		snprintf(msg, sizeof(msg), "msg after %d sleep", r);
		msgq_write(p->p_msg_q, p->signal, msg);
	}
	return NULL;
}

static void init_mqueue(struct coll_msg_t *mq)
{
	mq->w_index = mq->r_index = mq->items = 0;
	pthread_mutex_init(&mq->m_index, NULL);
	return;
}

int main()
{
	int i;
	struct parms_generator_t p_g[n_gen];
	struct parms_collector_t p_c;
	pthread_t tid[n_gen + 1];
	pthread_attr_t attr;

	/* initialize the message queueu */
	for (i = 0; i < n_gen; i++) {
		init_mqueue(&mqueues[i]);
	}
	/* init the signal mech. */
	pthread_cond_init(&signal_collector, NULL);
	/* init the generator parms. */
	for (i = 0; i < n_gen; i++) {
		p_g[i].signal = &signal_collector;
		p_g[i].p_msg_q = &mqueues[i];
	}
	/* init the collector parm */
	strncpy(p_c.fname, "/tmp/collector", sizeof(p_c.fname));
	p_c.signal = &signal_collector;
	p_c.n_queues = n_gen;
	p_c.p_msg_queues = mqueues;

	pthread_attr_init(&attr);
	/* create the collector thread. */
	pthread_create(&tid[n_gen], &attr, collector, &p_c);
	for (i = 0; i < n_gen; i++) {
		pthread_create(&tid[i], &attr, generator, &p_g[i]);
	}
	for (i = 0; i < n_gen; i++) {
		pthread_join(tid[i], NULL);
		printf("Thread %d done.\n", i);
	}
	/* TBD send hup to collector */
	return 0;
}

