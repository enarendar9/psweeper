#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

struct start_opts_t {
	int		proto;	/* udp = 0, tcp = 1 */
	const char	*addr;
	const char	*port;
	const char	*saddr; /* src addr to use */
	const char 	*sport; /* src port to use */
	int		range;  /* range of ports to sweep */
} cfg;


#define DADDR "127.0.0.1"
#define SADDR "127.0.0.1"
#define DPORT "5000"
#define SPORT "4999"
#define RANGE 2
#define PROTO 1

static void usage(char *name, char *err)
{
	printf("Error: %s\n", err);
	printf("Usage %s -d port [ -t|-u] [-a address] [ -p port ]"
	       " [ -s address ] [ -r dport-range]\n", name);
	printf("	-d = destination port\n");
	printf("	-a = destination address\n");
	printf("	-p = source port\n");
	printf("	-s = source address\n");
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
	int parsed_sport = 0;
	int parsed_range = 0 ;

	while((opt = getopt(argc, argv, "d:a:tus:p:r:h")) != -1) {
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

			case 'p':
				cfg.sport = optarg;
				parsed_sport = 1;
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
	if (!parsed_saddr)
		cfg.saddr = SADDR;
	if (!parsed_sport)
		cfg.sport = SPORT;
	if (!parsed_range)
		cfg.range = RANGE;
	return 0;
}

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

void client(struct start_opts_t *cfg_opts)
{
	struct addrinfo hints, *res, *src_res;
	int sfd;
	int i, rval;
	int retry = 4;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	if (cfg_opts->proto == 0)
		hints.ai_socktype = SOCK_DGRAM;
	if (cfg_opts->proto == 1)
		hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;


	/* get destination addr info */
	if (getaddrinfo(cfg_opts->addr, cfg_opts->port, &hints, &res) != 0)
		return;

	/* get source addr info */
	if (getaddrinfo(cfg_opts->saddr, cfg_opts->sport, &hints,
	    &src_res) != 0)
		return;
	for (i = 0; i < cfg_opts->range; i++) {
		if ((sfd = socket(res[0].ai_family, res[0].ai_socktype,
	    	    res[0].ai_protocol)) < 0) {
			printf("Not able to open socket to %s:%s(%d)\n",
			       cfg_opts->addr, cfg_opts->port, i);
			return;
		}
		/* bind to local port */
		if ((rval = do_bind(sfd, src_res[0].ai_addr,
		    src_res[0].ai_addrlen))) {
			printf("Not able to bind socket to %s:%s %s\n",
		               cfg_opts->saddr, cfg_opts->sport,
			       strerror(rval));
			return;
		}
		if (cfg_opts->proto == 1) {
			struct sockaddr_in *addr = (struct sockaddr_in *)res[0].ai_addr;
			if (i > 0)
				addr->sin_port += htons(1);
			if (connect(sfd, res[0].ai_addr, res[0].ai_addrlen) !=
			    -1) {
				printf("Connection to dest %s:%d worked\n",
					cfg_opts->addr, atoi(cfg_opts->port)+i);
			} else
				printf("Not able to connect to %s:%d (%s)\n",
					cfg_opts->addr, atoi(cfg_opts->port)+i,
					strerror(errno));
			close(sfd);
		} else {
			char t[] = "test";
			int try = 3;
			if (sendto(sfd, t, strlen(t), 0, res[0].ai_addr,
			    res[0].ai_addrlen)) {
				do {
					if (recvfrom(sfd, t, sizeof(t), MSG_DONTWAIT,
				    	    res[0].ai_addr, &res[0].ai_addrlen) > 0) {
						printf("Connection to dest [u] %s:%s worked\n",
						cfg_opts->addr, cfg_opts->port);
						break;
					} else {
						sleep(1);
						try--;
					}
				} while(try != 0);
			} else
				printf("Not able to connect to [u] %s:%s\n",
			       		cfg_opts->addr, cfg_opts->port);
			if (try == 0)
				printf("Not able to connect to [u] %s:%s\n",
			       		cfg_opts->addr, cfg_opts->port);
		}
	}
}

int main(int argc, char *argv[])
{
	/* create a tcp/udp socket at localhost
	 * for given port#
	 */
	parse_options(argc, argv);
	client(&cfg);
}
