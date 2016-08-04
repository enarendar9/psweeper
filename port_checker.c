/*
 * Check if a destination [tcp|udp] dest:port is open or not.
 */
#include <stdio.h>
#include "lib/msgq.h"

#define MAX_CHKRS 10

struct port_chkr_t {
	struct msgq_t *mq_tx[MAX_CHKRS];	/* send to tx-thread */
	struct msgq_t *mq_rx[MAX_CHKRS];	/* recv from rx-thread */
	pthread_cond_t *s2tx;	/* signal to tx-thread */
} port_chkrs;

/*
 * check's a peer for open port.
 */
struct checker_t {
	int		id;
	uint16_t	srcport;	/* host order */
	uint16_t	dstport;	/* will be set by dispatcher */
	int		tcp_state;
	uint32_t	seq;		/* host order */
	uint32_t	ack_seq;	/* host order */
	struct		tcphdr;
};

enum {
	TCP_SYN     = 0,
	TCP_SYN_ACK = 1,
	TCP_ACK_RST = 2,
	TCP_RST     = 3,
};

static void init_port_chkrs(pthread_cond_t *s2tx)
{
	int i;

	for (i = 0; i < MAX_CHKRS; i++) {
		port_chkrs.mq_tx[i] = init_msgq(s2tx, 0);
		port_chkrs.mq_rx[i] = init_msgq(NULL,
					msgq_flag_alloc | msgq_flag_readblock);
		port_chkrs[i].s2tx  = s2tx;
	}
	port_chkrs.s2tx  = s2tx;
}


static int send_pkt(int id, char *msg, size_t s)
{
	assert(id < MAX_CHKRS);
	return msgq_write(port_chkrs.mq_tx[i], msg, s);
}

/*
 * blocking call with a wait till we have a packet
 */
static inline int recv_pkt(int id, char *b, size_t s)
{
	int t = 1;
	int i = 0;
	s = msgq_read(port_chkrs.mq_rx[i], b, s);
	if (s <= 0)
		return 0;
	return s;
}

static void init_checker(struct checker_t *c, int id, uint16_t srcp,
			 uint16_t dstp)
{
	memset(c, 0, sizeof(*c));
	c->id = id;
	c->srcport = srcp;
	c->dstport = dport;
	c->tcpstate = TCP_SYN;
	c->seq = random() % ((1 << 32) - 1);
	setup_tcphdr(&c->tcphdr, srcp, dstp, c->seq, 0, c->tcpstate);
}

struct void setup_tcphdr(struct tcphdr *hdr, uint16_t src, uint16_t dst,
			 uint32_t seq, uint32_t ack_seq, enum state)
{
	hdr->source = htons(src);
	hdr->dest = htons(dst);
	switch (state) {
		case TCP_SYN:
			hdr->syn = 1;
			hdr->seq = htons(seq);
			break;
		case TCP_ACK_RST:
			hdr->ack = 1;
			hdr->rst = 1;
			hdr->psh = 1;
			hdr->ack_seq = htons(ack_seq);
			break;
		default:
			break;
	}
	set_chksum(hdr);
}

static int get_next_tcp_state(struct checker_t *c)
{
	switch(c->tcpstate) {
		case TCP_SYN_ACK:
			c->tcpstate = TCP_ACK_RST;
			break;
		default:
			break;
	}
}

static int set_tcp_state(struct checker_t *c, struct *tcphdr)
{
	if (tcphdr->syn == 1 && tcphdr->ack == 1) {
		c->ack_seq = ntohl(tcphdr->seq) + sizeof(*tcphdr);
		c->tcpstate = TCP_SYN_ACK;
		return 0;
	}
	return -1;
}

static int chk_rx_tcphdr(struct checker_t *c, struct *rxhdr)
{
	if (ntohs(rxhdr->source) ==  c->dstport &&
	    ntohs(rxhdr->dest) == c->srcport &&
	    ntohs(rxhdr->ack_seq) == c->seq)
		return 0;
}

static int do_check_port_open_tcp(struct checker_t *c)
{
	struct rx_tcphdr;
	int s, c = 1;

	send_pkt(c->id, &c->tcphdr, sizeof(c->tcphdr));
	c->seq += sizeof(c->tcphdr);
	do {
		s = recv_pkt(c->id, &rx_tcphdr, sizeof(rx_tcphdr));
		if (s <= 0)
			return -1;
		if (!chk_rx_tcphdr(c, &rx_tcpdhr))
			c = 0;
	} while (c == 1);
	if (set_tcp_state(c, rx_tcphdr) != 0)
		return -1;
	setup_tcphdr(&c->tcphdr, c->srcport, c->dstport, c->seq,
		     c->ack_seq, c->tcp_state);
	send_pkt(c->id, &c->tcphdr, sizeof(c->tcphdr));
	return 0;
}
