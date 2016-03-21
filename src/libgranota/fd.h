#ifndef FD_H
#define FD_H

#include <sys/types.h>
#include <sys/stat.h>

typedef int (*open_fn_t)(const char *pathname, int flags, ...);
typedef int (*close_fn_t)(int fd);
typedef int (*stat_fn_t)(const char *path, struct stat *buf);
typedef int (*access_fn_t)(const char *pathname, int mode);
typedef int (*__xstat_fn_t)(int ver, const char *path, struct stat *stat_buf);
typedef int (*__lxstat_fn_t)(int ver, const char *path, struct stat *buf);

struct fd_lib_fn {
	open_fn_t open;
	open_fn_t open64;
	close_fn_t close;
	stat_fn_t stat;
	access_fn_t access;
	__xstat_fn_t __xstat;
	__lxstat_fn_t __lxstat;
};

extern struct fd_lib_fn fd_fn;

extern int shadow_fd;

void fd_lib_init();

#endif /* FD_H */
