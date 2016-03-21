#ifndef EDIT_H
#define EDIT_H

#include "memfile.h"

#define UNDO_SIZE	16384

struct undo {
	size_t size;
	char buf[UNDO_SIZE];
};

int edit_replace(struct memfile *mf, struct undo *u, size_t len,
                 const void *search, const void *replace);
void edit_undo(struct undo *u);
void edit_finish(struct undo *u);

#endif /* EDIT_H */
