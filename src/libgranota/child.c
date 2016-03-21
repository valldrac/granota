#if HAVE_CONFIG_H
# include <config.h>
#endif

#define _GNU_SOURCE	/* get semtimedop */

#include "child.h"
#include "tracer.h"
#include "hash.h"

#include <common.h>
#include <semop.h>

#include <signal.h>
#include <err.h>

void
child_crash(int signum)
{
	if (child_pid == 0)
		kill(tracer_pid, SIGUSR1);
	raise(SIGSTOP);
}

void
wait_fuzzer(int semid)
{
	if (!tracer_pid)
		tracer_fork();
	if (sem_wait_child_test_target(semid) < 0)
		err(4, "sem_wait_child_test_target");
	if (sem_wait_tracer_post_child(semid) < 0)
		err(4, "sem_wait_tracer_post_child");
}

void
probe_memcmp(struct feedback *fdbk, const void *s1, const void *s2, size_t len)
{
	unsigned h1, h2;
	int seen1, seen2;
	int x1, x2;

	if (len < 3)
		return;

	h1 = atom_hash(s1, len);
	h2 = atom_hash(s2, len);

	seen1 = magic_hash_seen(&fdbk->history, h1);
	seen2 = magic_hash_seen(&fdbk->history, h2);
	if (seen1 && seen2)
		return;

	x1 = atom_new(&fdbk->magics, s1, len, h1);
	if (x1 < 0)
		return;
	x2 = atom_new(&fdbk->magics, s2, len, h2);
	if (x2 < 0)
		return;

	if (!seen1)
		magic_append(&fdbk->stack, fdbk->depth, x2, x1);
	if (!seen2)
		magic_append(&fdbk->stack, fdbk->depth, x1, x2);
}
