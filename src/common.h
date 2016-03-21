#ifndef COMMON_H
#define COMMON_H

#include <debug.h>

#include <stddef.h>

#define container_of(ptr, type, member) ({ \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#define MAX(a, b)	(((a)>(b)) ? (a) : (b))
#define MIN(a, b)	(((a)<(b)) ? (a) : (b))

/*
 * Symbolic names for common function attributes. 
 */
#define EXPORT __attribute__((__visibility__("default")))
#define NORETURN __attribute__((__noreturn__))
#define INIT(prio) __attribute__((__constructor__(prio)))
#define FINI(prio) __attribute__((__destructor__(prio)))
#define HOT __attribute__((__hot__))
#define CONST __attribute__((__const__))
#define ALIGNED(x) __attribute__((aligned(x)))

/*
 * String helper macros, see http://gcc.gnu.org/onlinedocs/cpp/Stringification.html
 */
#define XSTR(x) STR(x)
#define STR(x) #x

#endif /* COMMON_H */
