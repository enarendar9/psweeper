/*
 * Thread(s) to send and receive a IP packet.
 */
#include <sys/iphdr.h>

struct port2buff_mapping_t {
	uint16_t	p_num;		/* key */
	int		rx_buf_id;	/* values */
};

struct ip_sock_t {
	int ip_sfd;
	int ai_family;
	struct sockaddr *ai_addr;
	size_t	ai_len;
		tx_buffs;
	int	n_txbuffs;
		rx_buffs;
	int	n_rxbuffs;
	struct	port2buff_mapping_t 	*p2b_map;
} ip_sock;


int init_ip_rxtx(int ai_family, struct sockaddr *ai_addr, size_t ai_len,
		 struct msgq_t *tx_buffs, int n_txbuffs,
		 struct msgq_t *rx_buffs, int n_rxbuffs)
{
	ip_sock.ai_family = ai_family;
	ip_sock.ai_addr = ai_addr;
	ip_sock.ai_len = ai_len;
	ip_sock.sfd = socket(ai_family, SOCK_RAW, 0);
	bind(ip_sock.sfd, ai_addr, ai_len);
	/* 
	 * init the port2buf mapping
	 */
	ip_sock.p2b_map = calloc(n_rxbuffs, sizeof(struct port2buff_mappint_t));
	return 0;
}

/* thread to read pkts for circular buffer(s)
 * and send them on the ip-socket
 */
static void *tx_thread(void *arg)
{
}

/*
 * thread to read pkts from socket 
 * identify the tcp/udp hdr,
 * get the mapping from dstport to rx-buf
 * put the packet on the appropriate rx-buf
 */
static void *rx_thread(void *arg)
{
}
