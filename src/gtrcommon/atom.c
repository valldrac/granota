#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "atom.h"
#include "hash.h"

#include <common.h>
#include <atomic.h>

#include <string.h>

static char *
atom_alloc(struct set *set, int len)
{
	int off;

	off = atomic_faa(&set->bufsz, len);
	if (off + len <= SET_BUFSZ)
		return set->buf + off;
	else
		return NULL;
}

unsigned
atom_hash(const char *str, int len)
{
	return hash32(str, len, 0);
}

int
atom_new(struct set *set, const char *str, int len, unsigned hash)
{
	int i, j, n;
	struct atom *p;

	j = hash % ATOM_MAX;
	
	for (n = 0; n < ATOM_MAX; n++) {
		i = (n + j) % ATOM_MAX;
		p = &set->member[i];
		if (p->len == 0)
			break;
		if (p->hash == hash)
			return i;
	}
	if (unlikely(n == ATOM_MAX))
		return -1;

	p->str = atom_alloc(set, len);
	if (p->str == NULL)
		return -1;
	p->len = len;
	p->hash = hash;
	memcpy(p->str, str, len);
	return i;
}

inline struct atom *
atom_member(struct set *set, int i)
{
	return &set->member[i];
}

inline void
atom_empty(struct set *set)
{
	memset(set->member, 0, sizeof(set->member));
	atomic_null(&set->bufsz);
}
