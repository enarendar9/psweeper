/*
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
	const char server[] = "8.8.8.8";
	int err;
	struct sockaddr_in s, l;
	socklen_t l_len = sizeof(l);
	int sfd = socket(AF_INET, SOCK_RAW, 0);

	s.sin_family = AF_INET;
	s.sin_addr.s_addr = inet_addr(server);
	s.sin_port = htons(1);
	err = connect(sfd, (struct sockaddr *)&s, sizeof(s));
	err = getsockname(sfd, (struct sockaddr *)&l, &l_len);
	char buffer[100];
	const char *p = inet_ntop(AF_INET, &l.sin_addr, buffer, sizeof(buffer));
	if (p)
		printf("Use %s to connect to %s\n", p, server);
	else
		printf("not able to find a addr to connect to %s\n", server);
}
