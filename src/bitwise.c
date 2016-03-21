#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "bitwise.h"
#include "common.h"
#include "jkiss.h"
#include "strerr.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <alloca.h>

#define WSIZE 	sizeof(word_t)
#define WMASK 	(WSIZE - 1)
#define BPW	(WSIZE * CHAR_BIT)

static inline word_t
bswap(word_t w)
{
	switch (BPW) {
	case 64:
		return __builtin_bswap64(w);
	case 32:
		return __builtin_bswap32(w);
	}
}

static inline int
bclz(word_t w)
{
	return __builtin_clzll(w);
}

static word_t
randword()
{
	switch (BPW) {
	case 64:
		return ((word_t) jkiss32() << 32) | jkiss32();
	case 32:
		return jkiss32(); 
	}
}

static inline size_t
bit_index(size_t n)
{
	return P2NPHASE(n + 1, CHAR_BIT) + P2ALIGN(P2PHASE(n, BPW), CHAR_BIT);
}

struct bit *
bit_new()
{
	struct bit *set;

	set = malloc(sizeof(struct bit));
	if (unlikely(set == NULL)) {
		strerr_warnsys("malloc error");
		return NULL;
	}
	return bit_init(set);
}

struct bit *
bit_init(struct bit *set)
{
	set->len = 0;
	set->nwords = 0;
	set->nwalloc = 0;
	set->words = NULL;
	return set;
}

int
bit_resize(struct bit *set, size_t length)
{
	size_t nwords;
	word_t *words;

	nwords = P2ROUNDUP(length, BPW) / BPW;
	if (nwords > set->nwalloc) {
		words = calloc(nwords, WSIZE);
		if (unlikely(words == NULL)) {
			strerr_warnsys("calloc error");
			return -1;
		}
		memcpy(words, set->words, set->nwords);
		free(set->words);
		set->nwalloc = nwords;
		set->words = words;
	}
	set->nwords = nwords;
	set->len = length;
	return 0;
}

void
bit_clear(struct bit *set)
{
	free(set->words);
	bit_init(set);
}

void
bit_free(struct bit *set)
{
	free(set->words);
	free(set);
}

inline size_t
bit_length(const struct bit *set)
{
	return set->len;
}

inline size_t
bit_buf_len(const struct bit *set)
{
	return P2ROUNDUP(set->len, CHAR_BIT) / CHAR_BIT;
}

inline void *
bit_buf_get(struct bit *set)
{
	return set->words;
}

int
bit_get(const struct bit *set, size_t n)
{
	size_t i;

	i = n / BPW;
	if (unlikely(i >= set->nwords))
		return 0;
	return (set->words[i] >> bit_index(n)) & 1;
}

int
bit_eq(const struct bit *s, const struct bit *t)
{
	const word_t *sw, *tw;
	const word_t *se, *te;
	word_t w;
	size_t i;

	sw = s->words;
	tw = t->words;
	i = MAX(s->nwords, t->nwords);
	se = sw + s->nwords;
	te = tw + t->nwords;

	while (i--) {
		w = 0;
		if (likely(sw < se))
			w = *sw++;
		if (likely(tw < te))
			w ^= *tw++;
		if (w)
			return 0;
	}
	return 1;
}

ssize_t
bit_differ(const struct bit *s, const struct bit *t)
{
	const word_t *sw, *tw;
	const word_t *se, *te;
	word_t w;
	size_t i, max;
	int b;

	sw = s->words;
	tw = t->words;
	max = MAX(s->nwords, t->nwords);
	se = sw + s->nwords;
	te = tw + t->nwords;

	for (i = 0; i < max; i++) {
		w = 0;
		if (likely(sw < se))
			w = *sw++;
		if (likely(tw < te))
			w ^= *tw++;
		if (w) {
			b = bclz(bswap(w));
			return i * BPW + b;
		}
	}
	return -1;
}

static const word_t one = 1ULL;

inline void
bit_set(struct bit *set, size_t n)
{
	set->words[n / BPW] |= one << bit_index(n);
}

inline void
bit_toggle(struct bit *set, size_t n)
{
	set->words[n / BPW] ^= one << bit_index(n);
}

inline void
bit_unset(struct bit *set, size_t n)
{
	set->words[n / BPW] &= ~(one << bit_index(n));
}

static void
bit_copy_align(const char *restrict src, size_t foff, char *restrict dst,
               size_t toff, size_t len)
{
	unsigned char mask;
	size_t remain, n;
	int shift, adj;

	/*
	 * Align the source to the destination bit boundary.
	 */
	shift = P2PHASE(toff, CHAR_BIT);
	adj = P2PHASEUP(foff, CHAR_BIT, shift);
	if (adj - foff >= len)
		return;
	remain = len - adj + foff;

	src += adj / CHAR_BIT;
	dst += toff / CHAR_BIT;

	if (shift) {
		mask = WMASK >> shift;
		shift = CHAR_BIT - shift;
		if (remain > shift) {
			remain -= shift;
		} else {
			mask ^= mask >> remain;
			remain = 0;
		}
		*(dst++) = *(src++) & mask | (*dst & ~mask);
	}

	n = remain / CHAR_BIT;
	memcpy(dst, src, n);
	
	/*
	 * Finish off any remaining bits.
	 */
	shift = P2PHASE(remain, CHAR_BIT);
	if (shift) {
		dst += n;
		src += n;
		mask = WMASK >> shift;
		*dst = *src & ~mask | (*dst & mask);
	}

}

void
bit_copy(struct bit *from, size_t foff, struct bit *to,
         size_t toff, size_t len)
{
	if (len == 0)
		return;

	bit_copy_align(bit_buf_get(from), foff, bit_buf_get(to), toff, len);
}

void
bit_random(struct bit *set, size_t off, size_t len)
{
	size_t nwords;
	void *buf;
	word_t *p;

	if (len == 0)
		return;

	nwords = P2ROUNDUP(len, BPW) / BPW + 1;
	buf = alloca(nwords * WSIZE);
	p = buf;
	do {
		*(p++) = randword();
	} while (--nwords);

	bit_copy_align(buf, 0, bit_buf_get(set), off, len);
}

int
bit_read(int fd, struct bit *set)
{
	char *buf;
	size_t size, nbytes;
	ssize_t ret;

	size = bit_buf_len(set);
	buf = bit_buf_get(set);
	nbytes = 0;

	do {
		if (nbytes + BUFSIZ > size) {
			if (size < BUFSIZ)
				size = BUFSIZ;
			else
				size += size;
			ret = bit_resize(set, size * CHAR_BIT);
			if (unlikely(ret < 0))
				return ret;
			buf = bit_buf_get(set) + nbytes;
		}
		ret = read(fd, buf, BUFSIZ);
		if (unlikely(ret == -1))
			return ret;
		buf += ret;
		nbytes += ret;
	} while (ret);

	return bit_resize(set, nbytes * CHAR_BIT);
}

int
bit_write(int fd, struct bit *set)
{
	char *buf;
	size_t left;
	ssize_t written;

	left = bit_buf_len(set);
	buf = bit_buf_get(set);

	while (left > 0) {
		written = write(fd, buf, left);
		if (unlikely(written == -1))
			return written;
		buf += written;
		left -= written;
	}
	return 0;
}

