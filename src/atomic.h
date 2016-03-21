#ifndef ATOMIC_H
#define ATOMIC_H

#include <sched.h>

/*
 * Atomic operations for lock-free data structures.
 */
#define atomic_cas		__sync_bool_compare_and_swap
#define atomic_faa		__sync_fetch_and_add
#define atomic_null(x)		__sync_fetch_and_and(x, NULL)
#define atomic_barrier		__sync_synchronize
#define atomic_add		__sync_add_and_fetch
#define atomic_sub		__sync_sub_and_fetch
#define atomic_inc(x)		__sync_add_and_fetch(x, 1)
#define atomic_dec(x)		__sync_sub_and_fetch(x, 1)
#define atomic_or		__sync_or_and_fetch
#define atomic_and		__sync_and_and_fetch

/*
 * Naive spinlock for small critical sections.
 */
typedef volatile int spinlock_t;

/*
 * Grab a lock.
 */
static inline void
spin_lock(spinlock_t *lock)
{
	while (__sync_lock_test_and_set(lock, 1))
        	while (*lock)
			;
}

/*
 * Grab a lock without blocking.
 */
static inline int
spin_trylock(spinlock_t *lock)
{
	return (__sync_lock_test_and_set(lock, 1) == 0) ? 0 : -1;
}

/*
 * Release a lock.
 */
static inline void
spin_unlock(spinlock_t *lock)
{
	__sync_lock_release(lock);
}

/*
 * Yield is non-portable:
 *	Mac OS has pthread_yield_np()
 *	On Linux, thread_yield is implemented as a call to sched_yield(2)
 */
#if defined __APPLE__
# define thread_yield	pthread_yield_np()
#else
# define thread_yield	sched_yield
#endif

#endif	/* ATOMIC_H */
