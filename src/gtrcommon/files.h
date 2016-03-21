#ifndef FILES_H
#define FILES_H

#include <common.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define FILES_MAX	16

struct files {
	int num;
	int fds;
	int fdarray[FILES_MAX];
	char templ[PATH_MAX];
};

static inline char *
files_path(struct files *f, char *buf, int n)
{
	sprintf(buf, f->templ, n);
	return buf;
}

static inline int
files_fd(const struct files *f, int n)
{
	return f->fdarray[n];
}

static int
files_open(struct files *f, int flags, ...)
{
	int fd;
	mode_t mode;
	char buf[PATH_MAX];

	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}
	for (f->fds = 0; f->fds < f->num; f->fds++) {
		if (flags & O_CREAT)
			fd = open(files_path(f, buf, f->fds), flags, mode);
		else
			fd = open(files_path(f, buf, f->fds), flags);
		if (unlikely(fd == -1))
			break;
		f->fdarray[f->fds] = fd;
	}
	return fd;
}

static int
files_creat(struct files *f, mode_t mode)
{
	return files_open(f, O_CREAT | O_RDWR | O_TRUNC, mode);
}

static void
files_unlink(struct files *f)
{
	int n;
	char buf[PATH_MAX];

	for (n = f->fds; --n >= 0; )
		unlink(files_path(f, buf, n));
}

static void
files_close(struct files *f)
{
	int n;

	for (n = f->fds; --n >= 0; f->fds--)
		close(files_fd(f, n));
}

#endif /* FILES_H */
