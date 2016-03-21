#ifndef NULIST_H
#define NULIST_H

#include "common.h"
#include "strerr.h"

#include <stdlib.h>
#include <string.h>

/*
 * String, pointer and integer array-based lists.
 * For performance scenarios.
 */

/*
 * Table allocator alloc-ahead chunk; performance (+) / memory (-) trade-off
 */
#define ALLOC_CHUNK	128

struct nlist {
	size_t c;
	char **v;
};

static inline void
nlist_init(struct nlist *list)
{
	list->v = 0;
	list->c = 0;
}

static struct nlist *
nlist_new()
{
	struct nlist *list;

	list = malloc(sizeof(struct nlist));
	if (unlikely(list == NULL))
		strerr_warnsys("malloc error");
	else
		nlist_init(list);
	return list;
}

static inline int
nlist_add(struct nlist *list, char *name)
{
	char **v;

	if (unlikely(list->c % ALLOC_CHUNK == 0)) {
		v = realloc(list->v,
		            (1 + ALLOC_CHUNK + list->c) * sizeof(*list->v));
		if (unlikely(v == NULL)) {
			strerr_warnsys("realloc error");
			return -1;
		}
		list->v = v;
	}
	list->v[list->c++] = name;
	list->v[list->c] = 0;
	return 0;
}

static inline int
nlist_add_copy(struct nlist *list, const char *name)
{
	char *s = strdup(name);
	if (unlikely(s == NULL)) {
		strerr_warnsys("malloc error");
		return -1;
	}
	nlist_add(list, s);
	return 0;
}

static inline void
nlist_empty(struct nlist *list)
{
	if (list->v)
		free(list->v);
	nlist_init(list);
}

static inline void
nlist_free_copies(struct nlist *list)
{
	size_t i;
	for (i = 0; i < list->c; i++) {
		if (list->v[i])
			free(list->v[i]);
	}
	nlist_empty(list);
}

struct plist {
	size_t c;
	void **v;
};

static inline void
plist_init(struct plist *list)
{
	list->v = 0;
	list->c = 0;
}

static inline int
plist_add(struct plist *list, void *obj)
{
	void **v;

	if (unlikely(list->c % ALLOC_CHUNK == 0)) {
		v = realloc(list->v,
		            (1 + ALLOC_CHUNK + list->c) * sizeof(*list->v));
		if (unlikely(v == NULL)) {
			strerr_warnsys("realloc error");
			return -1;
		}
		list->v = v;
	}
	list->v[list->c++] = obj;
	list->v[list->c] = 0;
	return 0;
}

static inline void
plist_empty(struct plist *list)
{
	if (list->v)
		free(list->v);
	plist_init(list);
}

struct ulist {
	size_t c;
	unsigned long *v;
};

static inline void
ulist_init(struct ulist *list)
{
	list->v = 0;
	list->c = 0;
}

static inline int
ulist_add(struct ulist *list, unsigned long val)
{
	unsigned long *v;

	if (unlikely(list->c % ALLOC_CHUNK == 0)) {
		v = realloc(list->v,
		            (ALLOC_CHUNK + list->c) * sizeof(*list->v));
		if (unlikely(v == NULL)) {
			strerr_warnsys("realloc error");
			return -1;
		}
		list->v = v;
	}
	list->v[list->c++] = val;
	return 0;
}

static inline void
ulist_empty(struct ulist *list)
{
	if (list->v)
		free(list->v);
	ulist_init(list);
}

static inline void
ulist_sort(struct ulist *list)
{
	size_t i, j;
	unsigned long val;

	/*
	 * Insertion sort algorithm.
	 */
	for (i = 1; i < list->c; ++i) {
		val = list->v[i];
		j = i;
		while (j > 0 && val < list->v[i-1]) {
			list->v[j] = list->v[j-1];
			j--;
		}
		list->v[j] = val;
	}
}

#endif /* NULIST_H */
