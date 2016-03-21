#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "critbit.h"
#include "common.h"
#include "strerr.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct critbit_node {
	void *child[2];
	size_t bit;
};

int
critbit_inner(void *node)
{
	return ((uintptr_t) node & 1);
}

void
critbit_init(struct critbit_tree *t)
{
	t->root = NULL;
}

static void
_critbit_deltree(void *node)
{
	struct critbit_node *q;

	if (critbit_inner(node)) {
		q = node - 1;
		_critbit_deltree(q->child[0]);
		_critbit_deltree(q->child[1]);
		free(q);
	}
}

void
critbit_deltree(struct critbit_tree *t)
{
	_critbit_deltree(t->root);
}

int
critbit_contains(const struct critbit_tree *t, const struct bit *key)
{
	void *p;
	struct critbit_node *q;

	p = t->root;
	if (unlikely(p == NULL))
		return 0;

	while (critbit_inner(p)) {
		q = p - 1;
		p = q->child[bit_get(key, q->bit)];
	}
	return bit_eq(p, key);
}

int
critbit_insert(struct critbit_tree *t, struct bit *key, size_t *bitpos,
               void ***node)
{
	size_t n;
	int dir;
	void *p;
	struct critbit_node *q, *newnode;
	void **wherep;

	p = t->root;
	if (unlikely(p == NULL)) {
		*bitpos = 0;
		t->root = key;
		*node = &t->root;
		return 0;
	}

	while (critbit_inner(p)) {
		q = p - 1;
		p = q->child[bit_get(key, q->bit)];
	}

	n = bit_differ(p, key);
	if (unlikely(n == -1))
		return -2;

	dir = bit_get(p, n);

	newnode = malloc(sizeof(struct critbit_node));
	if (unlikely(newnode == NULL)) {
		strerr_warnsys("malloc error");
 		return -1;
	}
	
	*bitpos = newnode->bit = n;
	newnode->child[1 - dir] = key;
	*node = &newnode->child[1 - dir];

	wherep = &t->root;
	for (;;) {
		p = *wherep;
		if (!critbit_inner(p))
			break;
		q = p - 1;
		if (q->bit > n)
			break;
		wherep = &q->child[bit_get(key, q->bit)];
	}
	
	newnode->child[dir] = *wherep;
	*wherep = (void *) newnode + 1;
	return 0;
}

int
critbit_allprefixed(void *node,
                    int (*handle)(struct bit *, void *), void *arg)
{
	struct critbit_node *q;
	int ret;

	if (critbit_inner(node)) {
		q = node - 1;
		ret = critbit_allprefixed(q->child[0], handle, arg);
		if (ret)
			return ret;
		ret = critbit_allprefixed(q->child[1], handle, arg);
	} else {
		ret = handle(node, arg);
	}
	return ret;
}

int
critbit_traverse(struct critbit_tree *t,
                 int (*handle)(struct bit *, void *), void *arg)
{
	return critbit_allprefixed(t->root, handle, arg);
}

