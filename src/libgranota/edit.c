#if HAVE_CONFIG_H
# include <config.h>
#endif

#define _GNU_SOURCE	/* get memmem */

#include "edit.h"

#include <common.h>

#include <string.h> 

static inline size_t
undo_avail(const struct undo *u)
{
	return sizeof(u->buf) - u->size;
}

static inline int
undo_isempty(const struct undo *u)
{
	return u->size == 0;
}

static inline void
undo_push(struct undo *u, const void *e, size_t len)
{
	memcpy(u->buf + u->size, e, len);
	u->size += len;
}

static inline void
undo_pop(struct undo *u, void *e, size_t len)
{
	u->size -= len;
	memcpy(e, u->buf + u->size, len);
}

static inline void
undo_clear(struct undo *u)
{
	u->size = 0;
}

int
edit_replace(struct memfile *mf, struct undo *u, size_t len,
             const void *search, const void *replace)
{
	char *start, *end, *ptr;
	size_t needed;

	if (search == NULL || replace == NULL)
		return -1;

	needed = sizeof(search) + sizeof(ptr) + sizeof(len);
	if (undo_avail(u) < needed)
		return -1;

	start = mf->addr + mf->offset;
	end = start + mf->len;

	ptr = memmem(start, end - start, search, len);
	if (ptr == NULL)
		return 0;

	undo_push(u, &search, sizeof(search));
	undo_push(u, &ptr, sizeof(ptr));
	undo_push(u, &len, sizeof(len)); 
	memcpy(ptr, replace, len);
	start = ptr + len;
	return 1;
}

void
edit_undo(struct undo *u)
{
	char *ptr, *orig;
	size_t len;

	if (undo_isempty(u))
		return;

	undo_pop(u, &len, sizeof(len)); 
	undo_pop(u, &ptr, sizeof(ptr));
	undo_pop(u, &orig, sizeof(orig));
	memcpy(ptr, orig, len); 
}

void
edit_finish(struct undo *u)
{
	undo_clear(u);
}
