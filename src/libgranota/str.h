#ifndef STR_H
#define STR_H

#include <string.h>

#undef strcmp
#undef strncmp
#undef memcmp

typedef int (*strcmp_fn_t)(const char *s1, const char *s2);
typedef int (*strncmp_fn_t)(const char *s1, const char *s2, size_t n);
typedef int (*strcasecmp_fn_t)(const char *s1, const char *s2);
typedef int (*strncasecmp_fn_t)(const char *s1, const char *s2, size_t n);
typedef int (*strcoll_fn_t)(const char *s1, const char *s2);
typedef int (*memcmp_fn_t)(const void *s1, const void *s2, size_t n);
/* bcmp() is an obsolete alias for memcmp() */

struct str_lib_fn {
	strcmp_fn_t strcmp;
	strncmp_fn_t strncmp;
	strcasecmp_fn_t strcasecmp;
	strncasecmp_fn_t strncasecmp;
	strcoll_fn_t strcoll;
	memcmp_fn_t memcmp;
};

extern struct str_lib_fn str_fn;

void str_lib_init();

#endif /* STR_H */
