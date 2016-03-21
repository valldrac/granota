#ifndef BITWISE_H
#define BITWISE_H

#include <unistd.h>

/*
 * Macros for various sorts of alignment and rounding when the alignment
 * is known to be a power of two.
 */
#define P2ALIGN(x, align)		((x) & -(align))
#define P2PHASE(x, align)		((x) & ((align) - 1))
#define P2NPHASE(x, align)		(-(x) & ((align) - 1))
#define P2ROUNDUP(x, align)		(-(-(x) & -(align)))
#define P2END(x, align)			(-(~(x) & -(align)))
#define P2PHASEUP(x, align, phase)	((phase) - (((phase) -(x)) & -(align)))
#define P2CROSS(x, y, align)		(((x) ^ (y)) > (align) - 1)

/*
 * Type matching the CPU word's size.
 */
typedef size_t word_t;

/*
 * Bit vector.
 */
struct bit {
	size_t len;
	size_t nwords;
	size_t nwalloc;
	word_t *words;
};

struct bit *bit_new();
struct bit *bit_init(struct bit *set);
int bit_resize(struct bit *set, size_t length);
void bit_clear(struct bit *set);
void bit_free(struct bit *set);
size_t bit_length(const struct bit *set);
size_t bit_buf_len(const struct bit *set);
void *bit_buf_get(struct bit *set);
int bit_get(const struct bit *set, size_t n);
int bit_eq(const struct bit *s, const struct bit *t);
ssize_t bit_differ(const struct bit *s, const struct bit *t);
void bit_set(struct bit *set, size_t n);
void bit_toggle(struct bit *set, size_t n);
void bit_unset(struct bit *set, size_t n);
void bit_copy(struct bit *from, size_t foff, struct bit *to,
              size_t toff, size_t len);
void bit_random(struct bit *set, size_t off, size_t len);
int bit_read(int fd, struct bit *set);
int bit_write(int fd, struct bit *set);

#endif /* BITWISE_H */
