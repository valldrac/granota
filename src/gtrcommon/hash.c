#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "hash.h"

#include <common.h>

/*
 * Bitwise left rotation.
 */
static inline uint32_t
rol32(uint32_t i, int n)
{

	return (i << n | i >> (32 - n));
}

/*
 * Bitwise right rotation.
 */
static inline uint32_t
ror32(uint32_t i, int n)
{

	return (i << (32 - n) | i >> n);
}

/*
 * Simple implementation of the Murmur3-32 hash function.
 *
 * This implementation is slow but safe.  It can be made significantly
 * faster if the caller guarantees that the input is correctly aligned for
 * 32-bit reads, and slightly faster yet if the caller guarantees that the
 * length of the input is always a multiple of 4 bytes.
 */
uint32_t
hash32(const void *data, size_t len, uint32_t seed)
{
	const uint8_t *bytes;
	uint32_t hash, k;
	size_t res;

	/*
	 * Initialization.
	 */
	bytes = data;
	res = len;
	hash = seed;

	/*
	 * Main loop.
	 */
	while (res >= 4) {
		/*
		 * Block read. If your platform needs to do endian-swapping or
		 * can only handle aligned reads, do the conversion here.
		 */
		k = *(uint32_t *) bytes;
		bytes += 4;
		res -= 4;
		k *= 0xcc9e2d51;
		k = rol32(k, 15);
		k *= 0x1b873593;
		hash ^= k;
		hash = rol32(hash, 13);
		hash *= 5;
		hash += 0xe6546b64;
	}

	/*
	 * Remainder. Remove if input length is a multiple of 4.
	 */
	if (res > 0) {
		k = 0;
		switch (res) {
		case 3:
			k |= bytes[2] << 16;
		case 2:
			k |= bytes[1] << 8;
		case 1:
			k |= bytes[0];
			k *= 0xcc9e2d51;
			k = rol32(k, 15);
			k *= 0x1b873593;
			hash ^= k;
			break;
		}
	}

	/*
	 * Finalize.
	 */
	hash ^= (uint32_t) len;
	hash ^= hash >> 16;
	hash *= 0x85ebca6b;
	hash ^= hash >> 13;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16;

	return hash;
}

