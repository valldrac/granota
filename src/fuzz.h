#ifndef FUZZ_H
#define FUZZ_H

#include "ga.h"
#include "bandit.h"
#include "bitwise.h"
#include "nulist.h"

#include <files.h>
#include <input.h>
#include <session.h>
#include <target.h>
#include <feedback.h>

#include <time.h>

#define FUZZ_TMPDIR	"/dev/shm"

#define FUZZ_TIMEOUT	1000
#define FUZZ_ASLIMIT	512
#define FUZZ_MAXSIZE	(1024 * 1024)
#define FUZZ_BACKLOG	4
#define FUZZ_INTERVAL	2

#define FUZZ_STAGES	4

enum fuzz_stage {
	STG_SEED = 0,
	STG_RANDOM = 1,
	STG_MUTATE = 2,
	STG_CROSSOVER = 3,
};

struct fuzz_info {
	int tpid;
	unsigned long cycles[FUZZ_STAGES];
	unsigned long evals[FUZZ_STAGES];
	unsigned long colls[FUZZ_STAGES];
	unsigned long scores[FUZZ_STAGES];
	unsigned long refs[FUZZ_STAGES];
	int crashes[CRASH_TYPES];
	int stgnum;
	size_t nodecnt;
	size_t pivot;
	double pivotscore;
	double pivottime;
	struct bit *pivotstr;
	size_t pivotcritbit;
	unsigned poolsize;
};

struct fuzz_ctx {
	struct ga_context *ga;
	struct ga_pool *pool;
	struct karmed *nodes;
	struct bit *tmp[2];
	struct input *input;
	struct session *session;
	int shmid;
	int semid;
	unsigned head;
	unsigned tail;
	int queued;
	int backlog;
	int iter;
	int stgnum;
	size_t pivot;
	double sumscore;
	struct files files;
	struct nlist samples;
	struct plist bits;
	unsigned long cycles[FUZZ_STAGES];
	unsigned long evals[FUZZ_STAGES];
	unsigned long colls[FUZZ_STAGES];
	unsigned long scores[FUZZ_STAGES];
	unsigned long refs[FUZZ_STAGES];
	double reward[FUZZ_STAGES];
	int maxbits;
	int maxsize;
	int interval;
	int done;
	int stopping;
	timer_t timerid;
	int indirfd;
	int outdirfd;
	char *indir;
	char *outdir;
	char *outpfx;
	char *crashdir;
	char *tmpdir;
	char *workdir;
	char *exe;
	int tmout;
	int aslimit;
	int quiet;
	void (*callback)(void);
};

struct fuzz_ctx *fuzz_create();
int fuzz_setup(struct fuzz_ctx *ctx, struct target *tgt);
int fuzz_start(struct fuzz_ctx *ctx, void (*callback)(void));
int fuzz_stop(struct fuzz_ctx *ctx);
int fuzz_read_info(struct fuzz_ctx *ctx, struct fuzz_info *info);
void fuzz_destroy(struct fuzz_ctx *ctx);

#endif /* FUZZ_H */
