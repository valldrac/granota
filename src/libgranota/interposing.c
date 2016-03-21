#if HAVE_CONFIG_H
# include <config.h>
#endif

#define _GNU_SOURCE	/* get RTLD_NEXT */

#include <stddef.h>
#include <dlfcn.h>
#include <assert.h>

void *libc_dl_handle;
void *pthread_dl_handle;

static int handles_init;

int
dl_handles_init()
{
	if (handles_init)
		return -1;
	handles_init++;
	libc_dl_handle = RTLD_NEXT;
	if (dlsym(RTLD_NEXT, "pthread_create") != NULL)
		pthread_dl_handle = RTLD_NEXT;
	return 0;
}

void *
loadsym(void *handler, const char *symbol)
{
	void *p;
	char *err;

	dlerror();	/* clear any old error condition */
	p = dlsym(handler, symbol);
	err = dlerror();
	assert(err == NULL);
	return p;
}
