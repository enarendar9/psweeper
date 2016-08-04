/*
 * find a unused local address 
 * find a unused local sport
 */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#include <stdio.h>

static inline unsigned int get_port_num(void)
{
	struct timeval_t tv;
	int rval;

	gettimeofday(&tv, NULL);
	srandom((int)tv.usec);
	rval = random() % (1<<16);
	if (rval < 1024)
		rval += 1024;
	rval %= (1<<16);
	printf("port num generated %d\n", rval);
	return rval;
}

static int get_addr(const char *saddr, const *sport,
		    const int proto,
		    struct addrinfo *src_res)
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

	/* get source addr info */
	if (getaddrinfo(saddr, sport, &hints,
	    &src_res) != 0)
		return -1;
	return 0;
}

const char *get_unused_sport(const char *saddr, const int proto)
{
	unsigned int p;
	struct addrinfo *res;
	char s_port[10];
	int sfd, rval = 1;

	do {
		p = get_port_num();
		snprintf(s_port, sizeof(s_port), "%d", p);
		if (get_addr(saddr, s_port, proto, &res))
			return NULL;
		if ((sfd = socket(res[0].ai_family, res[0].ai_socktype,
	    	     res[0].ai_protocol)) < 0) {
			return NULL;
		if (!bind(sfd, res[0].ai_addr, res[0].ai_addr_len))
			rval = 0;
		close(sfd);
		freeaddrinfo(res);
	} while(rval);
	if (rval == 0) {
		char *rport = malloc(sizeof(s_port));
		strncpy(rport, s_port, sizeof(s_port));
		return rport;
	}
	return NULL;
}

/* 
 * from routing table find local address to use
 * to connect with destination address.
 */
const char *find_saddr(const char *addr)
{
}
