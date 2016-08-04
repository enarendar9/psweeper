#include <stdio.h>
#include <string.h>
#include "lib/msgq.h"
#include "consts.h"


struct parms_collector_t {
	pthread_mutex_t m_stop;
	int		stop;		/* when to exit the collector */
	int		n_gen;		/* # of queues from which to collect
					   messages */
	char		opfile[MAX_NAME];
	struct msgq_t	**mq;
};

struct parms_collector_t *init_parms_collector(struct msgq_t **p_m,
					       int n_workers, char *name)
{
	struct parms_collector_t *p_c = malloc(sizeof(*p_c));
	if (!p_c)
		return NULL;
	pthread_mutex_init(&p_c->m_stop, NULL);
	p_c->stop = 0;
	p_c->n_gen = n_workers;
	p_c->mq = p_m;
	strncpy(p_c->opfile, name, MAX_NAME);
	return p_c;
}

void collector_stop(struct parms_collector_t *p)
{
	pthread_mutex_lock(&p->m_stop);
	p->stop = 1;
	pthread_mutex_unlock(&p->m_stop);
}

void *collector(void *p)
{
	struct parms_collector_t *parms_c = p;
	FILE *fp;
	int s, i, stop = 0;
	char buff[MAX_MSG];

	if (!strcmp(parms_c->opfile, "stdout"))
		fp = stdout;
	else
		fp = fopen(parms_c->opfile, "w");
	if (fp == NULL)
		return;

	while (1) {
		for (i = 0; i < parms_c->n_gen; i++) {
			/* blocked waiting for the message. */
			s = msgq_read(parms_c->mq[i], buff, sizeof(buff));
			if (s > 0)
				fprintf(fp, "Queue %d: %s\n", i, buff); 
		}
		pthread_mutex_lock(&parms_c->m_stop);
		if (parms_c->stop == 1)
			stop = 1;
		pthread_mutex_unlock(&parms_c->m_stop);
		if (stop)
			break;
	}
	for (i = 0; i < parms_c->n_gen; i++) {
		/* don't wait for the message. */
		stop = 0;
		do {
			s = msgq_read_unblocked(parms_c->mq[i], buff,
						sizeof(buff));
			if (s > 0) {
				fprintf(fp, "Queue %d: %s\n", i, buff);
				if (msgq_is_empty(parms_c->mq[i]))
					stop = 1;
			} else
				stop = 1;
		} while (!stop);
	}
	fflush(fp);
	fclose(fp);
}
