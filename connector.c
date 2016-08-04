#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include "connector.h"


static int do_bind(int sfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int max_retry = 4;
	int retry = 0;
	int rval;

	do {
		if ((rval = bind(sfd, addr, addrlen))) {
			sleep(30);
			retry++;
		}
	} while(rval && retry < max_retry);
	return rval;
}

static int init_sock_values(const char *saddr, const char *sport,
			    const char *daddr, const char *dport,
			    const int proto,
			    struct addrinfo **res, struct addrinfo **src_res)
{
	struct addrinfo hints;
	int sfd;
	int i, rval;
	int retry = 4;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	if (proto == 0)
		hints.ai_socktype = SOCK_DGRAM;
	if (proto == 1)
		hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	/* get destination addr info */
	if (getaddrinfo(daddr, dport, &hints, res) != 0)
		return -1;

	/* get source addr info */
	if (sport != NULL) {
		if (getaddrinfo(saddr, sport, &hints,
		    src_res) != 0)
			return -1;
	} else
		*src_res = NULL;
	return 0;
}

static void get_local_addr_port(int af, int sfd,
				struct parms_work_func_t *cfg_opts)
{
	struct sockaddr	addr;
	socklen_t alen;

	if (cfg_opts->saddr) {
		free(cfg_opts->saddr);
		cfg_opts->saddr = NULL;
	}
	if (cfg_opts->sport) {
		free(cfg_opts->sport);
		cfg_opts->sport = NULL;
	}
	alen = sizeof(addr);
	if (getsockname(sfd, &addr, &alen))
		return;

	if (!(cfg_opts->saddr = malloc(INET6_ADDRSTRLEN)))
		return;
	if (!(cfg_opts->sport = malloc(10))) {
		free(cfg_opts->saddr);
		cfg_opts->saddr = NULL;
		return;
	}
	/* convert addr to string */
	struct sockaddr_in *s_in = (struct sockaddr_in *)&addr;
	inet_ntop(af, &s_in->sin_addr, cfg_opts->saddr, INET6_ADDRSTRLEN);
#if 0
	switch (af) {
		case AF_INET:
			inet_ntop(af, &((struct sockaddr_in *)&addr)->sin_addr,
				 cfg_opts->saddr, INET6_ADDRSTRLEN);
			break;

		case AF_INET6:
			inet_ntop(af,
				  &((struct sockaddr_in6 *)&addr)->sin6_addr,
				  cfg_opts->saddr, INET6_ADDRSTRLEN);
			break;

		default:
			free(cfg_opts->saddr);
			free(cfg_opts->sport);
			cfg_opts->saddr = NULL;
			cfg_opts->sport = NULL;
			return;
	}
	/* convert port to string */
	struct sockaddr_in *s_in = (struct sockaddr_in *)&addr;
#endif
	snprintf(cfg_opts->sport, 10, "%d", ntohs(s_in->sin_port));

	syslog(LOG_ERR, "%s: saddr: %s:%s\n", __FUNCTION__, cfg_opts->saddr,
	       cfg_opts->sport);
}


int connector_client(struct parms_work_func_t *cfg_opts, const char *dport)
{
	struct addrinfo *res, *src_res = NULL;
	int sfd;
	int i, rval = -1, brval;
	int retry = 4;

	if (init_sock_values(cfg_opts->saddr, cfg_opts->sport,
			     cfg_opts->daddr, dport, cfg_opts->proto,
			     &res, &src_res)) {
		syslog(LOG_ERR, "Not able to get addrinfo(s)\n");
		return rval;
	}
	if ((sfd = socket(res[0].ai_family, res[0].ai_socktype,
	    res[0].ai_protocol)) < 0) {
		syslog(LOG_ERR, "Not able to open socket to %s:%s(%d)\n",
			cfg_opts->daddr, dport, i);
		return rval;
	}

	/* bind to local port */
	if (src_res && (brval = do_bind(sfd, src_res[0].ai_addr,
	    src_res[0].ai_addrlen))) {
		syslog(LOG_ERR, "Not able to bind socket to %s:%s %s\n",
			cfg_opts->saddr, cfg_opts->sport,
			strerror(brval));
		freeaddrinfo(src_res);
		src_res = NULL;
	}
	if (cfg_opts->proto == 1) {
		if (connect(sfd, res[0].ai_addr, res[0].ai_addrlen) != -1) {
			rval = 0;
			if (src_res == NULL)
				get_local_addr_port(res[0].ai_family,
						    sfd, cfg_opts);
			syslog(LOG_DEBUG, "%s:%s (%s) via %s:%s %s\n",
			       cfg_opts->daddr, dport, strerror(errno),
		       	       cfg_opts->saddr, cfg_opts->sport,
		       	       "connected");
		} else {
			if (!cfg_opts->saddr ||
			    !cfg_opts->sport)
				syslog(LOG_DEBUG, "%s:%s (%s) %s\n",
				       cfg_opts->daddr, dport, strerror(errno),
		       		       "not connected");

			else
				syslog(LOG_DEBUG, "%s:%s (%s) via %s:%s %s\n",
				       cfg_opts->daddr, dport, strerror(errno),
		       		       cfg_opts->saddr, cfg_opts->sport,
		       		       "not connected");
		}
		close(sfd);
	} else {
		char t[] = "test";
		int try = 3;
		if (sendto(sfd, t, strlen(t), 0, res[0].ai_addr,
			   res[0].ai_addrlen)) {
			do {
				if (recvfrom(sfd, t, sizeof(t), MSG_DONTWAIT,
				    	     res[0].ai_addr,
					     &res[0].ai_addrlen) > 0) {
					syslog(LOG_DEBUG,
					       "Connection to dest [u] %s:%s"
					       " worked\n",
						cfg_opts->daddr, dport);
						break;
				} else {
					sleep(1);
					try--;
				}
			} while(try != 0);
		} else
			syslog(LOG_DEBUG, "Not able to connect to [u] %s:%s\n",
		       		cfg_opts->daddr, dport);
		if (try == 0)
			syslog(LOG_DEBUG, "Not able to connect to [u] %s:%s\n",
		       		cfg_opts->daddr, dport);
	}
	if (res)
		freeaddrinfo(res);
	if (src_res)
		freeaddrinfo(src_res);

	return rval;
}

int init_parms_work_func(struct parms_work_func_t *pwf,
			 int n_workers, struct msgq_t **p_m, const char *daddr,
		 	 const char *saddr, int proto)
{
	int i;
	for (i = 0; i < n_workers; i++) {
		pwf[i].p_m = p_m[i];
		pwf[i].daddr = daddr;
		pwf[i].proto = proto;
		if (saddr != NULL) {
			pwf[i].saddr = malloc(strlen(saddr)+1);
			if (pwf[i].saddr == NULL) {
				/* We will exit from the program. Heap leak ok.
				 */
				return -1;
			}
			strncpy(pwf[i].saddr, saddr, strlen(saddr));
		} else
			pwf[i].saddr = NULL;
		pwf[i].sport = NULL;
	}
	return 0;
}
