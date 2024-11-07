#define PROFILE_OP \
	X(PROF_PUT)		\
	X(PROF_GET)		\
	X(PROF_PGET)		\
	X(PROF_UPDATE)	\
	X(PROF_REMOVE)	\
	X(PROF_OP_UNKNOWN)

#define X(a) a,
enum { PROFILE_OP };
#undef X

void profile_enable(int enable);
void profile_gettime(struct timespec *ts);
int profile_enq(int op, void *key, size_t key_size, size_t value_size,
	size_t off, struct timespec *ts1, struct timespec *ts2);
void profile_dump(void);
