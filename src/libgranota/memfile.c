#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "memfile.h"

#include <common.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static long page_size;

int
memfile_map(struct memfile *mf, size_t offset, int fd)
{
	size_t len, pa_offset;
	char *addr;
	struct stat sb;
	
	if (unlikely(fstat(fd, &sb) < 0))
		return -1;

	if (offset >= sb.st_size) {
		errno = EINVAL;
		return -1;
	}
	len = sb.st_size - offset;
	
	if (unlikely(page_size == 0))
		page_size = sysconf(_SC_PAGE_SIZE);

	pa_offset = offset & ~(page_size - 1);

	addr = mmap(NULL, len + offset - pa_offset,
	            PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd, pa_offset);
	if (unlikely(addr == MAP_FAILED))
		return -1;

	mf->addr = addr;
	mf->offset = offset - pa_offset;
	mf->len = len;
	mf->fd = fd;
	return 0;
}

void
memfile_unmap(struct memfile *mf)
{
	munmap(mf->addr, mf->offset + mf->len);
	mf->addr = NULL;
}
