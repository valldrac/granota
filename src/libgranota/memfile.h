#ifndef MEMFILE_H
#define MEMFILE_H

#include <stddef.h>

struct memfile {
	char *addr;
	size_t offset;
	size_t len;
	int fd;
};

int memfile_map(struct memfile *mf, size_t offset, int fd);
void memfile_unmap(struct memfile *mf);

#endif /* MEMFILE_H */
