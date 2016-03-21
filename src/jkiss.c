#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "jkiss.h"
#include "random.h"

/*
 * Random number variables for KISS.
 */
uint32_t x, y, z, w = 456789123, cc;

/*
 * 32-bit KISS generator with no multiply instructions.
 */
uint32_t
jkiss32()
{
	int32_t t;

	y ^= (y << 5);
	y ^= (y >> 7);
	y ^= (y << 22);
	t = z + w + cc;
	z = w;
	cc = t < 0;
	w = t & 2147483647;
	x += 1411392427;

	return x + y + w;
}

/*
 * Initialise KISS generator using SWB.
 */
void
jkiss32_init()
{
	x = rand01() * 0xFFFFFFFF;
	while (!(y = rand01() * 0xFFFFFFFF));	/* y must not be zero! */
	z = rand01() * 0xFFFFFFFF;
}
