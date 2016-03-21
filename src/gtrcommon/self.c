#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "self.h"

#include <common.h>

#include <unistd.h>
#include <errno.h>

/*
 * Get declaration of _NSGetExecutablePath on MacOS X 10.2 or newer.
 */
#if HAVE_MACH_O_DYLD_H
# include <mach-o/dyld.h>
#endif

/*
 * Get the current executable pathname
 */ 
int
self_path(char *buf, size_t *size)
{
	ssize_t ret;
#if HAVE_SYS_PROCFS_H
	ret = readlink("/proc/self/exe", buf, *size - 1);
	if (likely(ret != -1)) {
		buf[ret] = '\0';
		*size = ret;
		ret = 0;
	}
#elif HAVE_MACH_O_DYLD_H && HAVE__NSGETEXECUTABLEPATH
	/*
	 * On MacOS X 10.2 or newer, the function
	 * int _NSGetExecutablePath (char *buf, unsigned long *bufsize)
	 * can be used to retrieve the executable's full path.
	 */
	ret = _NSGetExecutablePath(buf, size);
	if (unlikely(ret))
		errno = ENAMETOOLONG;
#else
# error No self_path() implemented for this platform
#endif
	return ret;
}
