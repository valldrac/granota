#if HAVE_CONFIG_H
# include <config.h>
#endif

#define _GNU_SOURCE	/* get semtimedop */

#include "fuzz.h"
#include "summary.h"
#include "util.h"
#include "random.h"
#include "common.h"
#include "jkiss.h"
#include "strerr.h"

#include <shmem.h>
#include <semop.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <openssl/md5.h>

#define CLOCKID	CLOCK_MONOTONIC

static int
fuzz_read(int dirfd, const char *filename, struct bit *s)
{
	int fd, ret;

	fd = openat(dirfd, filename, O_RDONLY);
	if (fd == -1) {
		strerr_warnsys("%s", filename);
		return fd;
	}
	ret = bit_read(fd, s);
	if (ret < 0)
		strerr_warnsys("%s", filename);
	close(fd);
	return ret;
}

static int
fuzz_write(int dirfd, const char *filename, struct bit *s)
{
	int fd, ret;

	fd = openat(dirfd, filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd == -1) {
		strerr_warnsys("%s", filename);
		return fd;
	}
	ret = bit_write(fd, s);
	if (ret < 0)
		strerr_warnsys("%s", filename);
	close(fd);
	return ret;
}


static int
fuzz_enqueue(struct fuzz_ctx *ctx, struct bit *s)
{
	int fd;

	fd = files_fd(&ctx->files, ctx->tail);
	if (lseek(fd, 0, SEEK_SET) != 0) {
		strerr_warnsys("input %u: seek error", ctx->tail);
		return -1;
	}
	if (ftruncate(fd, 0) < 0) {
		strerr_warnsys("input %u: truncate error", ctx->tail);
		return -1;
	}

	if (bit_write(fd, s) < 0) {
		strerr_warnsys("input %u: write error", ctx->tail);
		return -1;
	}
	if (sem_post_input(ctx->semid) < 0) {
		strerr_warnsys("cannot increment the input semaphore");
		return -1;
	}
	ctx->queued++;
	ctx->tail++;
	ctx->tail %= ctx->files.num;
	return 0;
}

static int
fuzz_output(struct fuzz_ctx *ctx, struct bit *s)
{
	unsigned char md[16];
	char name[33];

	MD5(bit_buf_get(s), bit_buf_len(s), md);
	snprintf(name, sizeof(name),
		 "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		 md[0], md[1], md[2], md[3], md[4], md[5], md[6], md[7],
		 md[8], md[9], md[10], md[11], md[12], md[13], md[14], md[15]);
	return fuzz_write(ctx->outdirfd, name, s);
}

static int
fuzz_dequeue(struct fuzz_ctx *ctx)
{
	unsigned head;
	int fd, ret;
	struct bit *s;
	double score;
	ssize_t n;

	if (sem_wait_result(ctx->semid) < 0) {
		strerr_warnsys("cannot decrement the result semaphore");
		return -1;
	}
	head = ctx->head++;
	ctx->head %= ctx->files.num;
	ctx->queued--;

	ctx->refs[ctx->stgnum] += ctx->input->tries[head];
	score = ctx->input->score[head];
	if (score <= 0)
		return 0;

	fd = files_fd(&ctx->files, head);
	if (lseek(fd, 0, SEEK_SET) != 0) {
		strerr_warnsys("input %u: seek error", head);
		return -1;
	}
	s = ga_new_string(ctx->iter);
	if (s == NULL) 
		return -1;

	if (bit_read(fd, s) < 0) {
		strerr_warnsys("input %u: read error", head);
		ga_free_string(s);
		return -1;
	}
	if (bit_length(s) == 0) {
		ga_free_string(s);
		return 0;
	}
	ret = ga_insert(ctx->ga, s, score);
	if (ret < 0) {
		ga_free_string(s);
		if (ret == -2) {
			ctx->colls[ctx->stgnum]++;
			ret = 0;
		}
		return ret;
	}
	if (plist_add(&ctx->bits, s) < 0)
		return -1;

	if (karmed_resize(ctx->nodes, ctx->bits.c) < 0)
		return -1;

	ctx->sumscore += score;
	ctx->scores[ctx->stgnum]++;

	if (ctx->outdir)
		fuzz_output(ctx, s);
	return 0;
}

static int
fuzz_evaluate(struct fuzz_ctx *ctx, struct bit *s)
{
	if (bit_length(s) == 0)
		return 0;
	if (!ga_unique(ctx->ga, s)) {
		ctx->colls[ctx->stgnum]++;
		return 0;
	}
	if (ctx->queued == ctx->files.num) {
		if (fuzz_dequeue(ctx) < 0)
			return -1;
		ctx->evals[ctx->stgnum]++;
	}
	if (fuzz_enqueue(ctx, s) < 0)
		return -1;
	return 0;
}

static int
fuzz_eval_pending(struct fuzz_ctx *ctx)
{
	while (ctx->queued) {
		if (fuzz_dequeue(ctx) < 0)
			return -1;
		ctx->evals[ctx->stgnum]++;
	}
	return 0;
}

static int
fuzz_crossover(struct fuzz_ctx *ctx)
{
	unsigned n;
	struct bit *p0, *p1, *p2, *c1, *c2;
	struct ga_pool *pool;

	p0 = ctx->bits.v[ctx->pivot];
	c1 = ctx->tmp[0];
	c2 = ctx->tmp[1];
	pool = ctx->pool;
	ctx->input->critbit = ga_critbit(p0);

	do {
		ga_genocide(pool);
		if (ga_map(ctx->ga, p0, (ga_map_callback_t) ga_populate,
		           pool) < 0)
			return -1;

		ga_fitness(pool);
		ga_select(pool);

		for (n = 0; n < pool->size - 1; n += 2) {
			p1 = pool->pop[n];
			p2 = pool->pop[n + 1];
			if (ga_crossover(ctx->ga, p1, p2, p0, c1, c2) < 0)
				return -1;
			if (bit_length(c1) > ctx->maxbits) {
				if (bit_resize(c1, ctx->maxbits) < 0)
					return -1;
			}
			if (bit_length(c2) > ctx->maxbits) {
				if (bit_resize(c2, ctx->maxbits) < 0)
					return -1;
			}
			if (fuzz_evaluate(ctx, c1) < 0)
				return -1;
			if (fuzz_evaluate(ctx, c2) < 0)
				return -1;
			if (ctx->done)
				break;
		}
	} while (!ctx->done);
	return 0;
}

static int
fuzz_mutate(struct fuzz_ctx *ctx)
{
	struct bit *p0;

	p0 = ctx->bits.v[ctx->pivot];
	ctx->input->critbit = ga_critbit(p0);

	do {
		if (ga_mutate(ctx->ga, p0, ctx->tmp[0]) < 0)
			return -1;
		if (fuzz_evaluate(ctx, ctx->tmp[0]) < 0)
			return -1;
	} while (!ctx->done);
	return 0;
}

static int
fuzz_random(struct fuzz_ctx *ctx)
{
	size_t len;

	ctx->input->critbit = 0;

	do {
		len = randinterval(1, ctx->maxbits);
		if (bit_resize(ctx->tmp[0], len) < 0)
			return -1;
		bit_random(ctx->tmp[0], 0, len);
		if (fuzz_evaluate(ctx, ctx->tmp[0]) < 0)
			return -1;
	} while (!ctx->done);
	return 0;
}

static int
fuzz_seed(struct fuzz_ctx *ctx)
{
	size_t n;

	ctx->input->critbit = 0;

	for (n = 0; n < ctx->samples.c; n++) {
		if (fuzz_read(ctx->indirfd, ctx->samples.v[n],
		              ctx->tmp[0]) < 0)
			return -1;
		if (bit_length(ctx->tmp[0]) > ctx->maxbits) {
			if (bit_resize(ctx->tmp[0], ctx->maxbits) < 0)
				return -1;
		}
		if (fuzz_evaluate(ctx, ctx->tmp[0]) < 0)
			return -1;
		if (ctx->stopping)
			break;
		ctx->callback();
	}
	nlist_free_copies(&ctx->samples);
	return 0;
}

static int
fuzz_run(struct fuzz_ctx *ctx)
{
	int stgnum, ret;
	size_t pivot;
	struct itimerspec its;
	struct timespec start, end, ts;
	double t;

	if (ctx->samples.c) {
		pivot = -1;
		stgnum = STG_SEED;
	} else {
		if (ctx->bits.c) {
			pivot = karmed_choose_best(ctx->nodes);
			if (ga_has_children(ctx->bits.v[pivot]))
				stgnum = STG_CROSSOVER;
			else
				stgnum = STG_MUTATE;
		} else {
			pivot = -1;
			stgnum = STG_RANDOM;
		}
	}
	ctx->iter++;
	ctx->stgnum = stgnum;
	ctx->pivot = pivot;
	ctx->sumscore = 0;
	ctx->done = 0;

	its.it_value.tv_sec = ctx->interval;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	timer_settime(ctx->timerid, 0, &its, NULL);

	clock_gettime(CLOCKID, &start);

	switch (stgnum) {
	case STG_SEED:
		ret = fuzz_seed(ctx);
		break;
	case STG_RANDOM:
		ret = fuzz_random(ctx);
		break;
	case STG_MUTATE:
		ret = fuzz_mutate(ctx);
		break;
	case STG_CROSSOVER:
		ret = fuzz_crossover(ctx);
		break;
	}
	if (ret == 0)
		ret = fuzz_eval_pending(ctx);

	clock_gettime(CLOCKID, &end);

	ctx->cycles[stgnum]++;
	if (stgnum == STG_MUTATE || stgnum == STG_CROSSOVER) {
		ts = difftimespec(start, end);
		t = ts.tv_sec + (double) ts.tv_nsec / 1000000000;
		karmed_reward(ctx->nodes, pivot, ctx->sumscore, t);
	}
	return ret;
}

static int
fuzz_dry_run(struct fuzz_ctx *ctx)
{
	bit_resize(ctx->tmp[0], 0);
	if (fuzz_enqueue(ctx, ctx->tmp[0]) < 0)
		return -1;
	return fuzz_dequeue(ctx);
}

static void
fuzz_timer_expire(union sigval sival)
{
	struct fuzz_ctx *ctx;
	
	ctx = sival.sival_ptr;
	ctx->done++;
}

struct fuzz_ctx *
fuzz_create()
{
	struct fuzz_ctx *ctx;

	ctx = calloc(sizeof(struct fuzz_ctx), 1);
	if (ctx == NULL) {
		strerr_warnsys("calloc error");
		return NULL;
	}

	ctx->shmid = -1;
	ctx->semid = -1;
	ctx->backlog = FUZZ_BACKLOG;
	ctx->pivot = -1;
	ctx->maxsize = FUZZ_MAXSIZE;
	ctx->interval = FUZZ_INTERVAL;
	ctx->indirfd = -1;
	ctx->outdirfd = -1;
	ctx->tmpdir = FUZZ_TMPDIR;
	ctx->tmout = FUZZ_TIMEOUT;
	ctx->aslimit = FUZZ_ASLIMIT;
	return ctx;
}

int
fuzz_setup(struct fuzz_ctx *ctx, struct target *tgt)
{
	time_t seed;
	struct sigevent sev;
	struct stat st;
	struct shm_desc shm;
	char path[PATH_MAX];

	ctx->maxbits = ctx->maxsize * CHAR_BIT;
	if (ctx->maxbits < 8) {
		strerr_warn("wrong input maximum size");
		return -1;
	}

	if (ctx->backlog < 2 || ctx->backlog >= FILES_MAX) {
		strerr_warn("wrong input queue size");
		return -1;
	}

	if (ctx->interval <= 0) {
		strerr_warn("wrong stage duration");
		return -1;
	}

	if (ctx->tmout < 50) {
		strerr_warn("bad or dangerously low value of -t");
		return -1;
	}

	if (ctx->aslimit < 10) {
		strerr_warn("bad or dangerously low value of -m");
		return -1;
	}

	switch (tgt->type) {
	case TGT_FILE:
		if (ctx->outpfx = strrchr(tgt->u.file.pathname, '/'))
			ctx->outpfx++;
		else
			ctx->outpfx = tgt->u.file.pathname;
		break;
	default:
		strerr_warn("target is not defined");
		return -1;
	}

	if (ctx->outpfx == NULL) {
		strerr_warn("invalid target file name");
		return -1;
	}

	randseed(time(&seed));
	printf("Random Seed: %lu\n", seed);
	jkiss32_init();

	if (ctx->indir) {
		ctx->indirfd = open(ctx->indir, O_RDONLY);
		if (ctx->indirfd == -1) {
			strerr_warnsys("cannot open directory '%s'", ctx->indir);
			return -1;
		}
		if (listdir(dup(ctx->indirfd), &ctx->samples) < 0)
			return -1;
		suffle(ctx->samples.v, ctx->samples.c);
		printf("Found %u sample files for analysis\n",
		       ctx->samples.c); 
	}
	if (ctx->outdir) {
		ctx->outdirfd = open(ctx->outdir, O_RDONLY);
		if (ctx->outdirfd == -1) {
			strerr_warnsys("cannot open directory '%s'", ctx->outdir);
			return -1;
		}
	}

	if (stat(ctx->exe, &st) < 0) {
		strerr_warnsys("%s", ctx->exe);
		return -1;
	}
	if (!(S_ISREG(st.st_mode) && (st.st_mode & 0111))) {
		strerr_warn("%s: is not executable", ctx->exe);
		return -1;
	}

	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = fuzz_timer_expire;
	sev.sigev_value.sival_ptr = ctx;
	sev.sigev_notify_attributes = NULL;
	if (timer_create(CLOCKID, &sev, &ctx->timerid) < 0) {
		strerr_warn("timer_create");
		return -1;
	}

	ctx->shmid = create_shm(ctx->exe, &shm);
	if (ctx->shmid == -1) {
		strerr_warnsys("cannot create SHM segment");
		return -1;
	}

	printf("Shared Memory Segment: %u (%llu bytes)\n",
	       ctx->shmid, shm.segsz);

	ctx->semid = create_sem();
	if (ctx->semid == -1) {
		strerr_warnsys("cannot create semaphore set");
		return -1;
	}

	printf("Semaphore Set: %u\n", ctx->semid);

	ctx->ga = ga_create();
	ctx->pool = ga_pool_new(32);
	ctx->nodes = karmed_new(32);
	ctx->tmp[0] = bit_new();
	ctx->tmp[1] = bit_new();

	if (ctx->ga == NULL
	    || ctx->pool == NULL
	    || ctx->nodes == NULL
	    || ctx->tmp[0] == NULL
	    || ctx->tmp[1] == NULL)
		return -1;

	memcpy(shm.target, tgt, sizeof(struct target));

	ctx->input = shm.input;
	ctx->input->semid = ctx->semid;
	ctx->session = shm.session;
	ctx->session->tmout = ctx->tmout;
	ctx->session->aslimit = ctx->aslimit;
	ctx->session->quiet = ctx->quiet;

	if (ctx->crashdir) {
		strncpy(ctx->session->crashdir, ctx->crashdir,
	        	sizeof(ctx->session->crashdir));
	} else {
		if (getcwd(path, sizeof(path)) == NULL) {
			strerr_warnsys("getpwd");
			return -1;
		}
		strcpy(ctx->session->crashdir, path);
	}

	if (ctx->tmpdir[0])
		snprintf(path, sizeof(path), "%s/queue-%u",
		         ctx->tmpdir, ctx->semid);
	else
		sprintf(path, "queue-%u", ctx->semid);

	if (mkdir(path, 0755) < 0) {
		strerr_warnsys("cannot mkdir '%s'", path);
		return -1;
	}
	ctx->workdir = strdup(path);
	ctx->files.num = ctx->backlog;
	ctx->input->files.num = ctx->backlog;
	sprintf(ctx->files.templ, "%s/%%.4x", path);
	sprintf(ctx->input->files.templ, "%s/%%.4x", path);

	printf("Writting files from '%s'", files_path(&ctx->files, path, 0));
	printf(" to '%s'\n", files_path(&ctx->files, path, ctx->files.num - 1));

	if (files_creat(&ctx->files, 0666) == -1) {
		strerr_warnsys("cannot open files");
		return -1;
	}
	return 0;
}

int
fuzz_start(struct fuzz_ctx *ctx, void (*callback)(void))
{
	if (fuzz_dry_run(ctx) < 0)
		return -1;

	ctx->callback = callback;

	while (!ctx->stopping) {
		if (fuzz_run(ctx) < 0)
			return -1;
		callback();
	}
	return 0;
}

int
fuzz_stop(struct fuzz_ctx *ctx)
{
	ctx->done++;
	ctx->stopping++;
	return 0;
}

int
fuzz_read_info(struct fuzz_ctx *ctx, struct fuzz_info *info)
{
	memset(info, 0, sizeof(struct fuzz_info));
	info->tpid = ctx->session->tpid;
	memcpy(info->crashes, ctx->session->crashes, sizeof(info->crashes));
	memcpy(info->cycles, ctx->cycles, sizeof(info->cycles));
	memcpy(info->evals, ctx->evals, sizeof(info->evals));
	memcpy(info->colls, ctx->colls, sizeof(info->colls));
	memcpy(info->scores, ctx->scores, sizeof(info->scores));
	memcpy(info->refs, ctx->refs, sizeof(info->refs));
	info->stgnum = ctx->stgnum;
	switch (ctx->stgnum) {
	case STG_CROSSOVER:
		info->poolsize = ctx->pool->size;
	case STG_MUTATE:
		info->pivot = ctx->pivot; 
		info->pivotscore = ctx->nodes->arms[ctx->pivot].reward;
		info->pivottime = ctx->nodes->arms[ctx->pivot].effort;
		info->pivotstr = ctx->bits.v[ctx->pivot];
		info->pivotcritbit = ga_critbit(info->pivotstr);
	}
	info->nodecnt = ctx->bits.c;
	return 0;
}

void
fuzz_destroy(struct fuzz_ctx *ctx)
{
	size_t n;

	if (ctx->bits.c) {
		for (n = 0; n < ctx->bits.c; n++)
			ga_free_string(ctx->bits.v[n]);
		plist_empty(&ctx->bits);
	}
	if (ctx->samples.c)
		nlist_free_copies(&ctx->samples);

	if (ctx->workdir) {
		files_unlink(&ctx->files);
		files_close(&ctx->files);
		rmdir(ctx->workdir);
		free(ctx->workdir);
	}
	if (ctx->semid != -1) {
		if (destroy_sem(ctx->semid) < 0)
			strerr_warnsys("cannot remove semaphore set");
	}
	if (ctx->shmid != -1) {
		if (destroy_shm(ctx->shmid) < 0)
			strerr_warnsys("cannot remove SHM segment");
	}
	if (ctx->timerid) 
		timer_delete(ctx->timerid);

	if (ctx->outdirfd != -1)
		close(ctx->outdirfd);

	if (ctx->indirfd != -1)
		close(ctx->indirfd);

	if (ctx->tmp[1])
		bit_free(ctx->tmp[1]);

	if (ctx->tmp[0])
		bit_free(ctx->tmp[0]);

	if (ctx->nodes)
		karmed_free(ctx->nodes);

	if (ctx->pool)
		ga_pool_free(ctx->pool);

	if (ctx->ga)
		ga_destroy(ctx->ga);

	free(ctx);
}
