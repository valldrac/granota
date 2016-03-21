#ifndef FEEDBACK_H
#define FEEDBACK_H

#include "atom.h"

#include <common.h>
#include <atomic.h>

#include <stdlib.h>

#define MAGIC_MAX	(ATOM_MAX / 2)

#define MAGIC_HASH_SET_MAX	1500

struct magic_pair {
	int priority;
	int atom[2];
};

struct magic_stack {
	int size;
	int top;
	struct magic_pair pair[MAGIC_MAX];
};

struct magic_hash_set {
	int size;
	unsigned hash[MAGIC_HASH_SET_MAX];
};

struct feedback {
	int depth;
	struct magic_stack stack;
	struct magic_hash_set history;
	struct set magics;
};

static int
magic_append(struct magic_stack *stack, int prio, int x, int y)
{
	int n, i;
	struct magic_pair *p;

	n = atomic_faa(&stack->size, 1);
	if (unlikely(n < 0))
		return -1;

	if (unlikely(n >= MAGIC_MAX)) {
		atomic_dec(&stack->size);
		return -1;
	}
	for (i = n; --i >= 0; ) {
		p = &stack->pair[i];
		if (p->atom[0] == x && p->atom[1] == y) {
			atomic_dec(&stack->size);
			return 0;
		}
	}

	p = &stack->pair[n];
	p->priority = prio;
	p->atom[0] = x;
	p->atom[1] = y;
	stack->top = -1;
	return 0;
}

static int
magic_pop(struct magic_stack *stack, int prio, int *x, int *y)
{
	int n;
	struct magic_pair *p;

	if (stack->top < 0)
		n = stack->size;
	else
		n = stack->top;

	if (unlikely(n >= MAGIC_MAX))
		n = MAGIC_MAX;

	while (--n >= 0) {
		p = &stack->pair[n];
		if (p->priority < 0) {
			stack->top = n;
			continue;
		}	
		if (p->priority < prio)
			break;	/* stack should be ordered */
		p->priority = -1;
		*x = p->atom[0];
		*y = p->atom[1];
		return 0;
	}
	return -1;
}

static inline void
magic_empty(struct magic_stack *stack)
{
	atomic_null(&stack->size);
	atomic_null(&stack->top);
}

static int
magic_hash_seen(struct magic_hash_set *set, unsigned hash)
{
	int n, size;

	size = set->size;
	if (unlikely(size > MAGIC_HASH_SET_MAX || size < 0))
		return 0;

	for (n = 0; n < size; n++) {
		if (set->hash[n] == hash)
			return 1;
	}
	return 0;
}

static int
magic_hash_new(struct magic_hash_set *set, unsigned hash)
{
	int n, size;

	size = set->size;
	if (unlikely(size > MAGIC_HASH_SET_MAX || size < 0))
		size = 0;	/* corrupted, reset */

	if (size == MAGIC_HASH_SET_MAX)
		return -1;

	for (n = 0; n < size; n++) {
		if (set->hash[n] == hash)
			return 0;
	}
	set->hash[n++] = hash;
	set->size = n;
	return 0;
}

#endif /* FEEDBACK_H */
