#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "coverage.h"
#include "libgranota.h"

#include <common.h>

#include <afl/config.h>

#include <sys/types.h>
#include <sys/shm.h>
#include <errno.h>

static int prev;
static int hits;

int
cvrg_init()
{
	return 0;
}

void
cvrg_collect()
{
	unsigned char *map;
	int i, x;

	map = shm.coverage;
	i = MAP_SIZE;
	x = 0;

	while (i--)
		x += *map++;
	hits = x;
}

double
cvrg_score()
{
	double score;

	if (hits == prev)
		return 0;
	score = (double) (hits - prev) / (MAP_SIZE - prev);
	prev = hits;
	return score;
}
