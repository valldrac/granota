#ifndef SESSION_H
#define SESSION_H

#include <atomic.h>

#include <unistd.h>
#include <limits.h>

#define CRASH_TYPES 6

enum crash_type {
	CRASH_ALREADY_SEEN = 0,
	CRASH_EXPLOITABLE = 1,
	CRASH_PROBABLY = 2,
	CRASH_PROBABLY_NOT = 3,
	CRASH_UNKNOWN = 4,
	CRASH_UNKNOWN_ERROR = 5,
};

struct session {
	pid_t tpid;
	int tmout;
	int aslimit;
	int quiet;
	int crashes[CRASH_TYPES];
	char crashdir[PATH_MAX];
};

static inline int
session_begin(struct session *sess, pid_t tpid)
{
	return atomic_cas(&sess->tpid, 0, tpid);
}

static inline void
session_end(struct session *sess)
{
	atomic_null(&sess->tpid);
}

static inline int
is_tracing(struct session *sess)
{
	return sess && sess->tpid != 0;
}

#endif /* SESSION_H */
