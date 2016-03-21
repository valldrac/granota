#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "str.h"
#include "libgranota.h"
#include "interposing.h"
#include "tracer.h"
#include "child.h"

#include <common.h>

struct str_lib_fn str_fn;

void
str_lib_init()
{
	str_fn.strcmp = loadsym(libc_dl_handle, "strcmp");
	str_fn.strncmp = loadsym(libc_dl_handle, "strncmp");
	str_fn.strcasecmp = loadsym(libc_dl_handle, "strcasecmp");
	str_fn.strncasecmp = loadsym(libc_dl_handle, "strncasecmp");
	str_fn.strcoll = loadsym(libc_dl_handle, "strcoll");
	str_fn.memcmp = loadsym(libc_dl_handle, "memcmp");
}

static size_t
strlen2(const char *s1, const char *s2)
{
	size_t len;

	for (len = 0; *s1++; len++) {
 		if (*s2++ == 0)
			return 0;
	}
	if (*s2)
		return 0;
	return len;
}

static size_t
strnlen2(const char *s1, const char *s2, size_t maxlen)
{
	size_t len;

	for (len = 0; len < maxlen; len++) {
		if (*s1 == 0 && *s1 == *s2)
			break;
		if (*s1++ == 0)
			return 0;
 		if (*s2++ == 0)
			return 0;
	}
	return len;
}

/*
 * String library function overrides.
 */

EXPORT
int
(strcmp)(const char *s1, const char *s2)
{
	int ret;

	if (unlikely(str_fn.strcmp == NULL))
		libgranota_init();

	ret = str_fn.strcmp(s1, s2);
	if (ret && tracer_pid)
		probe_memcmp(shm.fdbk, s1, s2, strlen2(s1, s2));
	return ret;
}

EXPORT
int
(strncmp)(const char *s1, const char *s2, size_t n)
{
	int ret;

	if (unlikely(str_fn.strncmp == NULL))
		libgranota_init();

	ret = str_fn.strncmp(s1, s2, n);
	if (ret && tracer_pid)
		probe_memcmp(shm.fdbk, s1, s2, strnlen2(s1, s2, n));
	return ret;
}

EXPORT
int
strcasecmp(const char *s1, const char *s2)
{
	int ret;

	ret = str_fn.strcasecmp(s1, s2);
	if (ret && tracer_pid)
		probe_memcmp(shm.fdbk, s1, s2, strlen2(s1, s2));
	return ret;
}

EXPORT
int
strncasecmp(const char *s1, const char *s2, size_t n)
{
	int ret;

	ret = str_fn.strncasecmp(s1, s2, n);
	if (ret && tracer_pid)
		probe_memcmp(shm.fdbk, s1, s2, strnlen2(s1, s2, n));
	return ret;
}

EXPORT
int
strcoll(const char *s1, const char *s2)
{
	int ret;

	ret = str_fn.strcoll(s1, s2);
	if (ret && tracer_pid)
		probe_memcmp(shm.fdbk, s1, s2, strlen2(s1, s2));
	return ret;
}

EXPORT
int
memcmp(const void *s1, const void *s2, size_t n)
{
	int ret;

	if (unlikely(str_fn.memcmp == NULL))
		libgranota_init();

	ret = str_fn.memcmp(s1, s2, n);
	if (ret && tracer_pid)
		probe_memcmp(shm.fdbk, s1, s2, n);
	return ret;
}

EXPORT
int
bcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}

