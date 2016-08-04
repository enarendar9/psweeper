/*
 * A simple circular message queue.
 */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define TIMEOUT 1
#define MAX_MSG 255
#define MSG_QUEUE 100

#define msgq_flag_alloc 	(1 << 0)
#define msgq_flag_readblock 	(1 << 1)

struct msg_t {
	char 	b[MAX_MSG];
	size_t	s_msg;
};

struct msgq_t {
	unsigned int	flags;
	struct msg_t	msg[MSG_QUEUE];
	int  		w_index;
	int  		r_index;
	int  		items;
	pthread_mutex_t m_index; 
	pthread_cond_t  *s_item; /* Signal when a msg is put into the queue */
};

/* Flags for msgq structures. */
#define msg_flag_alloc	(1<<0)

static inline struct msgq_t *init_msgq(pthread_cond_t *sig, unsigned int flags)
{
	struct msgq_t *mq = calloc(1, sizeof(*mq));
	if (mq == NULL)
		return NULL;
	pthread_mutex_init(&mq->m_index, NULL);
	if (flags & msg_flag_alloc) {
		mq->s_item = malloc(sizeof(pthread_cond_t));
		if (!mq->s_item) {
			free(mq);
			return NULL;
		}
		pthread_cond_init(mq->s_item, NULL);
		mq->flags |= msg_flag_alloc;
	} else if (sig != NULL) {
		mq->s_item = sig;
	}
	mq->flags = flags;
	return(mq);
}

static inline void free_msgq(struct msgq_t *mq)
{
	if (mq->flags & msg_flag_alloc)
		free(mq->s_item);
	free(mq);
}

/* Check if the message queue is empty?
 * Mutex is assumed to be held
 */
static inline int _msgq_is_empty(const struct msgq_t *mq)
{
	return (mq->items == 0);
}

static inline int msgq_is_full(const struct msgq_t *mq)
{
	return (mq->items == MSG_QUEUE);
}

static inline int msgq_write(struct msgq_t *mq, const char *msg, size_t s)
{
	struct msg_t *m;

	if (s > MAX_MSG)
		return -ENOMEM;

	pthread_mutex_lock(&mq->m_index);
	if (msgq_is_full(mq)) {
		pthread_mutex_unlock(&mq->m_index);
		return -ENOTEMPTY;
	}
	m = &mq->msg[mq->w_index];
	memcpy(m->b, msg, s);
	m->s_msg = s;
	mq->w_index = (mq->w_index + 1) % MSG_QUEUE;
	mq->items++;
	pthread_mutex_unlock(&mq->m_index);
	pthread_cond_broadcast(mq->s_item);
	return 0;
}

static int _msgq_read(struct msgq_t *mq, char *b, size_t s, int block)
{
	size_t mlen;
	struct timespec to;

	if (b == NULL)
		return -1;

	to.tv_sec = time(NULL) + TIMEOUT;
	to.tv_nsec = 0;

	pthread_mutex_lock(&mq->m_index);
	if (_msgq_is_empty(mq) && block) {
		if (mq->flags & msgq_flag_readblock) {
			pthread_cond_timedwait(mq->s_item, &mq->m_index, &to);
			if (_msgq_is_empty(mq)) {
				pthread_mutex_unlock(&mq->m_index);
				return -ENOENT;
			}
		} else {
			pthread_mutex_unlock(&mq->m_index);
			return -ENOENT;
		}
	}
	mlen = mq->msg[mq->r_index].s_msg;
	if (mlen > s) {
		pthread_mutex_unlock(&mq->m_index);
		return -ENOMEM;
	}
	memcpy(b, mq->msg[mq->r_index].b, mlen);
	mq->r_index = (mq->r_index + 1) % MSG_QUEUE;
	mq->items--;
	pthread_mutex_unlock(&mq->m_index);
	return mlen;
}

/*
 * return a message char string.
 * caller must already have allocated buffer (b) of size s.
 */
static inline int msgq_read(struct msgq_t *mq, char *b, size_t s)
{
	return _msgq_read(mq, b, s, 1);
}

static inline int msgq_read_unblocked(struct msgq_t *mq, char *b, size_t s)
{
	return _msgq_read(mq, b, s, 0);
}

static inline int msgq_is_empty(struct msgq_t *mq)
{
	int r;
	pthread_mutex_lock(&mq->m_index);
	r = _msgq_is_empty(mq);
	pthread_mutex_unlock(&mq->m_index);
	return r;
}
