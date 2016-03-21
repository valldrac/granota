#ifndef BANDIT_H
#define BANDIT_H

#include <stddef.h>

#define BANDIT_E_GREEDY	0.05	/* epsilon for e-greedy strategy */

struct arm {
	double reward;
	double effort;
};

struct karmed {
	struct arm *arms;
	size_t k;
	size_t reserved;
};

struct karmed *karmed_new(size_t k);
int karmed_resize(struct karmed *p, size_t k);
size_t karmed_choose_best(struct karmed *p);
void karmed_reward(struct karmed *p, size_t i, double amount, double effort);
void karmed_free(struct karmed *p);

#endif /* BANDIT_H */
