#ifndef CRITBIT_H
#define CRITBIT_H

#include "bitwise.h"

struct critbit_tree {
	void *root;
};

int critbit_inner(void *node);
void critbit_init(struct critbit_tree *t);
void critbit_deltree(struct critbit_tree *t);
int critbit_contains(const struct critbit_tree *t, const struct bit *key);
int critbit_insert(struct critbit_tree *t, struct bit *key,
                   size_t *bitpos, void ***node);
int critbit_allprefixed(void *node,
                        int (*handle)(struct bit *, void *), void *arg);
int critbit_traverse(struct critbit_tree *t,
                     int (*handle)(struct bit *, void *), void *arg);

#endif /* CRITBIT_H */
