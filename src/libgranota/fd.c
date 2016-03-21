#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "fd.h"
#include "libgranota.h"
#include "interposing.h"
#include "match.h"
#include "child.h"

#include <common.h>
#include <atomic.h>
#include <input.h>

#include <fcntl.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>

struct fd_lib_fn fd_fn;

int shadow_fd = -1;

void
fd_lib_init()
{
	fd_fn.open = loadsym(libc_dl_handle, "open");
	fd_fn.open64 = loadsym(libc_dl_handle, "open64");
	fd_fn.close = loadsym(libc_dl_handle, "close");
	fd_fn.stat = loadsym(libc_dl_handle, "stat");
	fd_fn.access = loadsym(libc_dl_handle, "access");
	fd_fn.__xstat = loadsym(libc_dl_handle, "__xstat");
	fd_fn.__lxstat = loadsym(libc_dl_handle, "__lxstat");
}

/*
 * File descriptor library function overrides.
 */

EXPORT
int
open(const char *pathname, int flags, ...)
{
	int fd;
	mode_t mode;
	char tmp[PATH_MAX];

	if (unlikely(fd_fn.open == NULL))
		libgranota_init();

	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
		fd = fd_fn.open(pathname, flags, mode);
	} else {
		if (((flags & O_ACCMODE) == O_RDONLY)
		    && match_tgt_file(shm.target, pathname)) {
			if (shadow_fd == -1)
				wait_fuzzer(shm.input->semid);
			shadow_fd = fd_fn.open(get_input_path(tmp, shm.input), flags);
			fd = shadow_fd;
		} else {
			fd = fd_fn.open(pathname, flags);
		}
	}
	return fd;
}

EXPORT
int
open64(const char *pathname, int flags, ...)
{
	int fd;
	mode_t mode;
	char tmp[PATH_MAX];

	if (unlikely(fd_fn.open == NULL))
		libgranota_init();

	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
		fd = fd_fn.open64(pathname, flags, mode);
	} else {
		if (((flags & O_ACCMODE) == O_RDONLY)
		    && match_tgt_file(shm.target, pathname)) {
			if (shadow_fd == -1)
				wait_fuzzer(shm.input->semid);
			shadow_fd = fd_fn.open64(get_input_path(tmp, shm.input), flags);
			fd = shadow_fd;
		} else {
			fd = fd_fn.open64(pathname, flags);
		}
	}
	return fd;
}

EXPORT
int
close(int fd)
{
	int ret;

	ret = fd_fn.close(fd);
	if (ret == 0 && shadow_fd == fd)
		shadow_fd = -1;
	return ret;
}

EXPORT
int
stat(const char *path, struct stat *buf)
{
	char tmp[PATH_MAX];

	if (unlikely(fd_fn.stat == NULL))
		libgranota_init();

	if (match_tgt_file(shm.target, path))
		return fd_fn.stat(get_input_path(tmp, shm.input), buf);
	else
		return fd_fn.stat(path, buf);
}

EXPORT
int
access(const char *pathname, int mode)
{
	char tmp[PATH_MAX];

	if (unlikely(fd_fn.access == NULL))
		libgranota_init();

	if (match_tgt_file(shm.target, pathname))
		return fd_fn.access(get_input_path(tmp, shm.input), mode);
	else
		return fd_fn.access(pathname, mode);
}

EXPORT
int
__xstat(int ver, const char *path, struct stat *buf)
{
	char tmp[PATH_MAX];

	if (unlikely(fd_fn.__xstat == NULL))
		libgranota_init();

	if (match_tgt_file(shm.target, path))
		return fd_fn.__xstat(ver, get_input_path(tmp, shm.input), buf);
	else
		return fd_fn.__xstat(ver, path, buf);
}

EXPORT
int
__lxstat(int ver, const char *path, struct stat *buf)
{
	char tmp[PATH_MAX];

	if (unlikely(fd_fn.__lxstat == NULL))
		libgranota_init();

	if (match_tgt_file(shm.target, path))
		return fd_fn.__lxstat(ver, get_input_path(tmp, shm.input), buf);
	else
		return fd_fn.__lxstat(ver, path, buf);
}
