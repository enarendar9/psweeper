#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

struct start_opts_t {
	int		proto;	/* udp = 0, tcp = 1 */
	const char	*addr;
	const char 	*port;
} cfg;

static void usage(char *name, char *err)
{
	printf("Error: %s\n", err);
	printf("Usage %s -f family -p port [ -t|-u] [-a address]\n", name);
	printf("	family = [v4|v6]\n");
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

void start_server(struct start_opts_t *cfg_opts)
{
	struct addrinfo hints, *res;
	struct sockaddr_storage peer;
	socklen_t peer_len = sizeof(peer);
	char host[NI_MAXHOST], service[NI_MAXSERV];
	int sfd, asfd;
	int r;

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

	if (getaddrinfo(cfg_opts->addr, cfg_opts->port, &hints, &res) != 0)
		return;
	if ((sfd = socket(res[0].ai_family, res[0].ai_socktype,
	    res[0].ai_protocol)) < 0)
		return;
	if (bind(sfd, res[0].ai_addr, res[0].ai_addrlen))
		return;
	printf("Listening for connections [%c] %s:%s\n",
		cfg_opts->proto?  't':'u', cfg_opts->addr, cfg_opts->port);
	if (cfg_opts->proto) {
		if (listen(sfd, 1)) {
			printf("closing %s %d\n", strerror(errno), errno);
			close(sfd);
			return;
		}
	}
	while (1) {
		if (cfg_opts->proto) {
			if ((asfd = accept(sfd,
			    (struct sockaddr *)&peer, &peer_len))) {
				if (!getnameinfo((struct sockaddr *)&peer,
					peer_len, host, NI_MAXHOST,
					service, NI_MAXSERV, NI_NUMERICSERV))
					printf("Accepted from: %s:%s\n",
						host, service);
				shutdown(asfd, SHUT_RDWR);
			}
		} else {
			char buf[200];
			if (recvfrom(sfd, buf, 200, 0,
			    (struct sockaddr *)&peer, &peer_len)) {
				if (!getnameinfo((struct sockaddr *)&peer,
					peer_len, host, NI_MAXHOST,
					service, NI_MAXSERV, NI_NUMERICSERV))
					printf("Accepted from: %s:%s\n",
						host, service);
				sendto(sfd, buf, 3, 0, (struct sockaddr *)&peer,
				       peer_len);
			}
		}
	}
	close(sfd);
}

int main(int argc, char *argv[])
{
	/* create a tcp/udp socket at localhost
	 * for given port#
	 */
	parse_options(argc, argv);
	start_server(&cfg);
}
