#ifndef SELF_H
#define SELF_H

#include <stddef.h>

int self_path(char *buf, size_t *size);

#if HAVE_SYS_PROCFS_H || (HAVE_MACH_O_DYLD_H && HAVE__NSGETEXECUTABLEPATH)
# define SELF_PATH_SUPPORTED 1
#endif

#endif /* SELF_H */
