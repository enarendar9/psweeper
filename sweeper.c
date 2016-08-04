/*
 * Program to check the tcp ports that are open at the given
 * destination IPv4/IPv6 address.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <syslog.h>

#include "lib/dispatcher.h"
#include "lib/freeblist.h"
#include "connector.h"
#include "consts.h"
#include "collector.h"

#define MAX_MSG 255
#define MSG_QUEUE 100

struct start_opts_t {
	int		proto;	/* udp = 0, tcp = 1 */
	const char	*addr;
	const char	*port;
	const char	*saddr; /* src addr to use */
	int		range;  /* range of ports to sweep */
	const char	*opfile;
} cfg;


#define DADDR "127.0.0.1"
#define SADDR "127.0.0.1"
#define DPORT "5000"
#define RANGE 1
/* TCP default */
#define PROTO 1

struct work_item_t {
	int	stop;
	char	dport[MAX_MSG];	/* port to check */
};

pthread_cond_t s2c;

#define MAX_WORKERS 10
const int n_workers = MAX_WORKERS;
static int do_work(void *p, char *c)
{
	struct parms_work_func_t *p_wf = p;
	struct work_item_t *p_work = (struct work_item_t *)c;
	int res;
	char msg[MAX_MSG];
	int retry = 4;
	int s_sec = 2;

	/* checking if we should quit. */
	if (p_work->stop)
		return -1;

	syslog(LOG_ERR, "%s: Checking %s:%s.\n", __FUNCTION__,
	       p_wf->daddr, p_work->dport);
	/* Check if the connection is open or not */
	res = connector_client(p_wf, p_work->dport);
	/* Send the result to the collector  */
	snprintf(msg, sizeof(msg), "%s:%s %s", p_wf->daddr, p_work->dport,
		 (res == 0) ? "open" : "closed");
	syslog(LOG_ERR, "Result: %s\n", msg);
	do {
		if (msgq_write(p_wf->p_m, msg, strlen(msg)+1) == -ENOTEMPTY) {
			sleep(s_sec);
			retry--;
		} else
			retry = 0;
	} while(retry > 0);
	return 0;
}


/*
 * return 0 if work item available.
 * return -1 if all done.
 */
static int work_item_get_next(struct work_item_t *i, int st_port,
			      struct blist_t *bl)
{
	int idx = blist_get_nextz(bl);
	if (idx < 0)
		return -1;
	blist_set(bl, idx);
	snprintf(i->dport, MAX_MSG,  "%d", st_port+idx);
	i->stop = 0;
	syslog(LOG_ERR, "Work port is %s", i->dport);
	return 0;
}

static void *dispatcher(void *p)
{
	struct parms_dispatcher_t *parms_d = p;
	int c;
	struct work_item_t item;
	struct worker2dispatcher_t *w2d = msg_dispatcher_get_w2d(parms_d->d);
	const struct start_opts_t *st_cfg = parms_d->cfg_disp;
	struct blist_t *bl;
	int start_port = atoi(st_cfg->port);

	bl = blist_init(st_cfg->range);

	c = parms_d->num_jobs; /*#of jobs */
	while (!work_item_get_next(&item, start_port, bl)) {
		/* will block if all are busy .. */
		if (dispatch_next(parms_d->d, (char *)&item)) {
			syslog(LOG_ERR, "Failed to dispatch work item.\n");
		}
	}
	/* Wait for all the pending tasks to be done. */
	wait_all_workers(parms_d->d);
	syslog(LOG_DEBUG, "Dispatcher: All workers are done now.\n");
	/* Stop all workers */
	item.stop = 1;
	item.dport[0] = '\0';
	dispatch_all_workers(parms_d->d, (char *)&item);
	syslog(LOG_DEBUG, "Dispatcher: All workers asked to stop.\n");
}

static void usage(char *name, char *err)
{
	printf("Error: %s\n", err);
	printf("Usage %s -d port [ -t|-u] [-a address] [ -p port ]"
	       " [ -s address ] [ -r dport-range]\n", name);
	printf("	-d = destination port\n");
	printf("	-a = destination address\n");
	printf("	-p = source port\n");
	printf("	-s = source address\n");
	printf("	-o = output file (default: stdout)\n");
	printf("	port = [1-65535]\n");
	printf("	-t|-u = TCP or UDP");
	printf("	address = [Ipv4 or Ipv6 address]\n");
	printf("	dport-range = port range to sweep default = 2\n");
	exit(-1);
}

static int parse_options(int argc, char *argv[])
{
	int opt;
	int parsed_addr = 0;
	int parsed_proto = 0;
	int parsed_port = 0;
	int parsed_saddr = 0;
	int parsed_range = 0;
	int parsed_opfile = 0;

	while((opt = getopt(argc, argv, "d:a:tus:r:ho:")) != -1) {
		switch(opt) {
			case 'd':
				cfg.port = optarg;
				parsed_port = 1;
				break;

			case 'a':
				if (parsed_addr)
					usage(argv[0], "Already setup addr");
				cfg.addr = optarg;
				parsed_addr = 1;
				break;

			case 's':
				if (parsed_saddr)
					usage(argv[0], "Already setup addr");
				cfg.saddr = optarg;
				parsed_saddr = 1;
				break;

			case 't':
			case 'u':
				if (parsed_proto)
					usage(argv[0], "Already setup protocol");
				if (opt == 't')
					cfg.proto = 1;
				else if (opt == 'u')
					cfg.proto = 0;
				parsed_proto = 1;
				break;

			case 'r':
				if (parsed_range)
					usage(argv[0], "Already setup range");
				cfg.range = atoi(optarg);
				parsed_range = 1;
				break;

			case 'h':
				usage(argv[0], "Help");
				break;

			case 'o':
				if (parsed_opfile)
					usage(argv[0], "Already setup output"
					      " file");
				cfg.opfile = optarg;
				parsed_opfile = 1;
				break;

			default:
				usage(argv[0], "unsupported option");
				break;
		}
	}
	if (!parsed_addr)
		cfg.addr = DADDR;
	if (!parsed_port)
		cfg.port = DPORT;
	if (!parsed_proto)
		cfg.proto = PROTO;
	if (!parsed_range)
		cfg.range = RANGE;
	if (!parsed_opfile)
		cfg.opfile = "stdout";
	if (!parsed_saddr)
		cfg.saddr = NULL;
	return 0;
}

int main(int argc, char *argv[])
{
	struct parms_worker_t p_w[n_workers+1];
	struct parms_work_func_t p_wf[n_workers];
	struct parms_dispatcher_t p_d;
	struct parms_collector_t  *p_c;
	struct msgq_t *p_m[n_workers];
	struct msg_dispatcher_t *mds;
	pthread_t tid[n_workers + 1];
	pthread_attr_t attr;
	int i, n_jobs;

	if (parse_options(argc, argv))
		return -1;

	openlog(argv[0], LOG_PID, LOG_USER);

	/* Init dispatcher. */
	mds = init_msg_dispatcher(n_workers, sizeof(struct work_item_t));

	/* init generators parms for workers */
	pthread_cond_init(&s2c, NULL);
	for (i = 0; i < n_workers; i++)
		p_m[i] = init_msgq(&s2c, msgq_flag_alloc|msgq_flag_readblock);

	if (init_parms_work_func(p_wf, n_workers, p_m, cfg.addr, cfg.saddr,
				 cfg.proto))
		return -1;

	void *a[n_workers];
	for (i = 0; i < n_workers; i++)
		a[i] = &p_wf[i];

	/* init the worker thread(s) parms. */
	init_parms_worker(mds, p_w, n_workers, do_work, a);

	/* init the dispatcher thread parms. */
	init_parms_dispatcher(&p_d, n_jobs, mds, &cfg);


	/* Init the collector thread params */
	p_c = init_parms_collector(p_m, n_workers, cfg.opfile);


	pthread_attr_init(&attr);

	/* start the collector thread. */
	pthread_create(&tid[n_workers+1], &attr, collector, p_c);

	/* start the workers. */
	for (i = 0; i < n_workers; i++) {
		pthread_create(&tid[i], &attr, worker, &p_w[i]);
	}
	/* create the generator thread. */
	pthread_create(&tid[n_workers], &attr, dispatcher, &p_d);

	/* Wait for dispatcher to be done */
	pthread_join(tid[n_workers], NULL);

	/* Wait for workers to be done */
	for (i = 0; i < n_workers; i++) {
		pthread_join(tid[i], NULL);
		syslog(LOG_DEBUG, "Thread %d done.\n", i);
	}
	collector_stop(p_c);
	pthread_join(tid[n_workers+1], NULL);
	syslog(LOG_DEBUG, "Collector done.\n");

	/* TBD: Clean up all the allocations. */

	closelog();
	return 0;
}
