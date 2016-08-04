/*
 * connector.h: Connect to a remote destip:port
 */
#include "lib/msgq.h"
struct parms_work_func_t {
	struct msgq_t *p_m; /* msgq used by the socket thread to
			       send results to the collector */
	const char	*daddr; /* destination to check */
	char		*saddr; /* src address to use */
	char		*sport; /* port to bind to */
	int		proto;  /* proto tcp(1)|udp(0) */
	unsigned int	flags;
};

#define wf_flag_fixed_ip (1<<0)
extern int connector_client(struct parms_work_func_t *cfg_opts,
			    const char *dport);
extern int init_parms_work_func(struct parms_work_func_t *pwf,
				int n_workers, struct msgq_t **p_m,
				const char *daddr, const char *saddr, int proto);
