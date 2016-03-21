#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "stream.h"
#include "libgranota.h"
#include "match.h"
#include "interposing.h"
#include "fd.h"

#include <common.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct stream_lib_fn stream_fn;

static int
sflags(const char *mode)
{
	int m, o;

	switch (*mode++) {
	case 'r':       /* open for reading */
		m = O_RDONLY;
		o = 0;
		break;
	case 'w':       /* open for writing */
		m = O_WRONLY;
		o = O_CREAT | O_TRUNC;
		break;
	case 'a':       /* open for appending */
		m = O_WRONLY;
		o = O_CREAT | O_APPEND;
		break;
	default:        /* illegal mode */
		return 0;
	}
	/*
	 * [rwa]\+ or [rwa]b\+ means read and write
	 */
	if (*mode == '+' || (*mode == 'b' && mode[1] == '+'))
		m = O_RDWR;
	/*
	 * XXX: GNU C library extensions
	 */
	else if (*mode == 'c' && mode[1] == 'e')
		o |= O_CLOEXEC;

	return m | o;
}

void
stream_lib_init()
{
	stream_fn.fopen = loadsym(libc_dl_handle, "fopen");
	stream_fn.fclose = loadsym(libc_dl_handle, "fclose");
}

/*
 * Stream-based I/O library function overrides.
 */

EXPORT
FILE *
fopen(const char *path, const char *mode)
{
	int fd;
	FILE *fp;

	fd = open(path, sflags(mode));
	if (fd >= 0)
		fp = fdopen(fd, mode);
	else
		fp = NULL;
	return fp;
}

EXPORT
int
fclose(FILE *fp)
{
	int ret, fd;

	fd = fileno(fp);
	ret = stream_fn.fclose(fp);
	if (ret == 0 && shadow_fd == fd)
		shadow_fd = -1;
	return ret;
}
