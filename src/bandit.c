#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "bandit.h"
#include "random.h"
#include "common.h"
#include "strerr.h"

#include <stdlib.h>

static int
karmed_grow(struct karmed *p, size_t needed)
{
	struct arm *arms;

	if (p->reserved > 0)
		needed += needed + 1;

	arms = realloc(p->arms, sizeof(struct arm) * needed);
	if (needed && arms == NULL) {
		strerr_warnsys("realloc error");
		return -1;
	}

	p->arms = arms;
	p->reserved = needed;
	return 0;
}

struct karmed *
karmed_new(size_t k)
{
	struct karmed *p;

	p = calloc(sizeof(struct karmed), 1);
	if (p == NULL) {
		strerr_warnsys("malloc error");
		return NULL;
	}
	if (karmed_resize(p, k) < 0) {
		free(p);
		return NULL;
	}
	return p;
}

int
karmed_resize(struct karmed *p, size_t k)
{
	size_t i;

	if (p->reserved < k) {
		if (karmed_grow(p, k) < 0)
			return -1;
	}
	for (i = p->k; i < k; i++) {
		p->arms[i].reward = 0;
		p->arms[i].effort = 0;
	}
	p->k = k;
}

void
karmed_reward(struct karmed *p, size_t i, double amount, double effort)
{
	p->arms[i].reward += amount;
	p->arms[i].effort += effort;
}

size_t
karmed_choose_best(struct karmed *p)
{
	size_t head, i, j, best;
	double mean, max;

	if (p->k == 1)
		return 0;

	/*
	 * Bootstrap: choose the first lever that has not yet
	 * been tested.
	 */
	for (i = 0; i < p->k; i++) {
		if (p->arms[i].effort == 0)
			return i;
	}

	/*
	 * Exploration: choose a random lever.
	 */
	best = randinterval(0, p->k - 1);

	/*
	 * Exploitation: choose the lever with the greatest
	 * expectation of reward.
	 */
 	if (rand01() >= BANDIT_E_GREEDY) {
		head = best;
		max = 0;
		for (j = 0; j < p->k; j++) {
			i = (head + j) % p->k;
			mean = p->arms[i].reward / p->arms[i].effort;
			if (max < mean) {
				best = i;
				max = mean;
			}
		}
	}
	return best;
}

void
karmed_free(struct karmed *p)
{
	free(p->arms);
	free(p);
}
