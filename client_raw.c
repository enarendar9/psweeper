#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

struct start_opts_t {
	int		proto;	/* udp = 0, tcp = 1 */
	const char	*srcaddr;
	const char	*addr;
	const char	*port;
} cfg;

static void usage(char *name, char *err)
{
	printf("Error: %s\n", err);
	printf("Usage %s -p port [ -t|-u] [-a address]\n", name);
	printf("	port = [1-65535]\n");
	printf("	-t|-u = TCP or UDP");
	printf("	address = [Ipv4 or Ipv6 address]\n");
	exit(-1);
}

static int parse_options(int argc, char *argv[])
{
	int opt;
	int parsed_addr = 0;
	int parsed_proto = 0;
	int parsed_port = 0;

	while((opt = getopt(argc, argv, "p:a:tu")) != -1) {
		switch(opt) {
			case 'p':
				cfg.port = optarg;
				parsed_port = 1;
				break;

			case 'a':
				if (parsed_addr)
					usage(argv[0], "Already setup addr");
				cfg.addr = optarg;
				parsed_addr = 1;
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
			default:
				usage(argv[0], "unsupported option");
				break;
		}
	}
	if (!parsed_addr)
		cfg.addr = "127.0.0.1";
	if (!parsed_port)
		cfg.port = "5000";
	if (!parsed_proto)
		cfg.proto = 1;
	return 0;
}

enum {
	TCP_SYN     = 0,
	TCP_ACK_RST = 1,
	TCP_RST = 2,
};

struct void setup_tcphdr(struct tcphdr *hdr, uint16_t src, uint16_t dst,
			 uint32_t seq, uint32_t ack_seq, enum state)
{
	hdr->source = htons(src);
	hdr->dest = htons(dst);
	switch (state) {
		case TCP_SYN:
			hdr->syn = 1;
			hdr->seq = seq;
			break;
		case TCP_ACK_RST:
			hdr->ack = 1;
			hdr->rst = 1;
			hdr->psh = 1;
			hdr->ack_seq = ack_seq;
			break;
		default:
			break;
	}
}

void client(struct start_opts_t *cfg_opts)
{
	struct addrinfo hints, *res, *srcres;
	int sfd;
	int i;
	struct tcphdr tcphdr;
	uint32_t seq = random() % ((1<<32))-1;
	uint32_t ack_seq;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	if (getaddrinfo(cfg_opts->srcaddr, NULL, &hints, &srcres) != 0)
		return;

	if ((sfd = socket(srcres[0].ai_family, SOCK_RAW, 0)) < 0) {
		printf("not able to create socket for %s.\n", cfg_opts->srcaddr);
	}
	if (bind(sfd, srcres[0].ai_addr, srcres[0].ai_addrlen))
		return;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	if (getaddrinfo(cfg_opts->addr, cfg_opts->port, &hints, &res) != 0)
		return;

	struct sockaddr_in *saddr = (struct sockaddr_in *)srcres[0].ai_addr;
	saddr->sin_port += htons(i);

	memset(&tcphdr, 0, sizeof(tcphdr));
	setup_tcphdr(&tcphdr, ntohs(saddr->sin_port), atoi(cfg_opts->port),
		     htonl(seq), 0, TCP_SYN); 

	for (i = 0; i < 2; i++) {
		if (i == 0 && (sfd = socket(res[0].ai_family, SOCK_RAW, 0)) < 0) {
			printf("Not able to connect to %s:%s(%d)\n",
				cfg_opts->addr, cfg_opts->port, i);
			return;
		}
		
		if (cfg_opts->proto == 1) {
			struct sockaddr_in *addr = (struct sockaddr_in *)res[0].ai_addr;
			addr->sin_port += htons(i);
			if (connect(sfd, res[0].ai_addr, res[0].ai_addrlen) !=
			    -1) {
				printf("Connection to dest %s:%s worked\n",
					cfg_opts->addr, cfg_opts->port);
			} else
				printf("Not able to connect to %s:%s\n",
					cfg_opts->addr, cfg_opts->port);
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
//		close(sfd);
	}
	close(sfd);
}

int main(int argc, char *argv[])
{
	/* create a tcp/udp socket at localhost
	 * for given port#
	 */
	parse_options(argc, argv);
	client(&cfg);
}
