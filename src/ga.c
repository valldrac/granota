#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "ga.h"
#include "random.h"
#include "common.h"
#include "strerr.h"

#include <stdlib.h>

struct individual {
	struct bit chrom;	/* actual chromosome string */
	size_t critbit;
	void **lineage;
	double rawscore;	/* objective function value */
};

static struct individual *
owner(struct bit *s)
{
	return container_of(s, struct individual, chrom);
}

static int
ga_pool_realloc(struct ga_pool *pool, size_t size)
{
	void *pop, *selected, *fitness;

	pop = realloc(pool->pop, size * sizeof(void *));
	selected = realloc(pool->selected, size * sizeof(unsigned));
	fitness = realloc(pool->fitness, size * sizeof(double));
	if (pop == NULL || selected == NULL || fitness == NULL) {
		strerr_warnsys("realloc error");
		free(fitness);
		free(selected);
		free(pop);
		return -1;
	}
	pool->pop = pop;
	pool->selected = selected;
	pool->fitness = fitness;
	pool->allocated = size;
	return 0;
}

struct ga_context *
ga_create()
{
	struct ga_context *ctx;

	ctx = malloc(sizeof(struct ga_context));
	if (ctx == NULL) {
		strerr_warnsys("malloc error");
		return NULL;
	}
	critbit_init(&ctx->phylo_tree);
	return ctx;
}

struct ga_pool *
ga_pool_new(unsigned size_hint)
{
	struct ga_pool *pool;

	pool = calloc(1, sizeof(struct ga_pool));
	if (pool == NULL) {
		strerr_warnsys("calloc error");
		return NULL;
	}
	if (ga_pool_realloc(pool, size_hint) < 0) {
		free(pool);
		return NULL;
	}
	pool->sigma_factor = GA_SIGMA_SCALING_COEFF;
	return pool;
}

void
ga_pool_free(struct ga_pool *pool)
{
	free(pool->fitness);
	free(pool->selected);
	free(pool->pop);
	free(pool);
}

struct bit *
ga_new_string()
{
	struct individual *ind;

	ind = malloc(sizeof(struct individual));
	if (ind == NULL) {
		strerr_warnsys("malloc error");
		return NULL;
	}
	return bit_init(&ind->chrom);
}

void
ga_free_string(struct bit *s)
{
	struct individual *ind;

	ind = owner(s);
	bit_clear(&ind->chrom);
	free(ind);
}

int
ga_has_children(struct bit *s)
{
	return critbit_inner(*owner(s)->lineage);
}

size_t
ga_critbit(struct bit *s)
{
	return MIN(owner(s)->critbit, bit_length(s));
}

void
ga_destroy(struct ga_context *ctx)
{
	critbit_deltree(&ctx->phylo_tree);
	free(ctx);
}

int
ga_insert(struct ga_context *ctx, struct bit *s, double score)
{
	struct individual *ind;

	ind = owner(s);
	ind->rawscore = score;
	return critbit_insert(&ctx->phylo_tree, &ind->chrom, &ind->critbit,
	                      &ind->lineage);
}

int
ga_unique(struct ga_context *ctx, struct bit *s)
{
	return critbit_contains(&ctx->phylo_tree, s) == 0;
}

int
ga_map(struct ga_context *ctx, struct bit *ancestor,
       ga_map_callback_t apply, void *arg)
{
	if (ancestor)
		return critbit_allprefixed(*owner(ancestor)->lineage,
 		                           apply, arg);
	else
		return critbit_traverse(&ctx->phylo_tree, apply, arg);
}

int
ga_populate(struct bit *s, struct ga_pool *pool)
{
	unsigned needed;

	needed = pool->size + 1;
	if (needed > pool->allocated) {
		if (ga_pool_realloc(pool, needed * 1.5) < 0)
			return -1;
	}
	pool->pop[pool->size] = s;
	pool->size = needed;
	sum_add(&pool->stats, owner(s)->rawscore);
	return 0;
}

void
ga_fitness(struct ga_pool *pool)
{
	double sigma, avg, adj, sum;
	int i;
	struct bit *s;

	sum = 0;
	sigma = sum_std(&pool->stats) * pool->sigma_factor;
	avg = sum_mean(&pool->stats);

	if (sigma != 0) {
		for (i = 0; i < pool->size; i++) {
			s = pool->pop[i];
			adj = 1 + (owner(s)->rawscore - avg) / sigma;
			if (adj <= 0)
				adj = 0;
			pool->fitness[i] = adj;
			sum += adj;
		}
	} else {
		for (i = 0; i < pool->size; i++) {
			pool->fitness[i] = 1;
			sum += 1;
		}
	}
	pool->fitavg = sum / pool->size;
}

void
ga_genocide(struct ga_pool *pool)
{
	pool->size = 0;
	pool->fitavg = 0;
	sum_clear(&pool->stats);
}

/*
 * A select routine using stochastic universal sampling.
 *
 * Ref: J. Baker, Reducing Bias and Inefficiency in the Selection Algorithm.
 * Second GA conference, pp 14-21 (page 16)
 */
void
ga_select(struct ga_pool *pool)
{
	int i, k;
	double sum;	/* running sum of expected values */
	float r;

	sum = 0;
	k = 0;
	r = rand01();

	for (i = 0; i < pool->size; i++)
		for (sum += pool->fitness[i] / pool->fitavg; sum > r; r++)
			pool->selected[k++] = i;
	suffle(pool->selected, pool->size);
}

static void
ga_bitflip(struct bit *s, size_t min, size_t max)
{
	int k;
	size_t n;

	if (min == max)
		return;
	k = randpoiss(1.0);
	while (k--) {
		n = randinterval(min, max);
		bit_toggle(s, n);
	}
}

int
ga_mutate(struct ga_context *ctx, struct bit *p, struct bit *c)
{
	size_t len, x, y;

	len = bit_length(p);
	x = randinterval(ga_critbit(p), len);
	y = x + randinterval(1, 64);
	if (len < y)
		len = y;
	if (bit_resize(c, len) < 0)
		return -1;
	bit_copy(p, 0, c, 0, x);
	bit_random(c, x, y - x);
	bit_copy(p, y, c, y, len - y);
	return 0;
}

static int 
ga_cross_1p(struct bit *s, struct bit *p1, size_t x, struct bit *p2)
{
	size_t len;

	len = bit_length(p2);
	if (bit_resize(s, len) < 0)
		return -1;
	bit_copy(p1, 0, s, 0, x);
	bit_copy(p2, x, s, x, len - x);
	return 0;
}

static int 
ga_cross_2p(struct bit *s, struct bit *p1, size_t x, size_t y, struct bit *p2)
{
	size_t len;

	len = bit_length(p1);
	if (bit_resize(s, len) < 0)
		return -1;
	bit_copy(p1, 0, s, 0, x);
	bit_copy(p2, x, s, x, y - x);
	bit_copy(p1, y, s, y, len - y);
	return 0;
}

static int 
ga_cut_splice(struct bit *s, struct bit *p1, size_t x, struct bit *p2, size_t y)
{
	size_t len;

	len = bit_length(p2);
	if (bit_resize(s, len - y + x) < 0)
		return -1;
	bit_copy(p1, 0, s, 0, x);
	bit_copy(p2, y, s, x, len - y);
	return 0;
}

int
ga_crossover(struct ga_context *ctx, struct bit *p1, struct bit *p2,
             struct bit *p0, struct bit *c1, struct bit *c2)
{
	float r;
	size_t len, len1, len2;
	size_t critbit, x, y;
	int ret1, ret2;

	len1 = bit_length(p1);
	len2 = bit_length(p2);
	len = MIN(len1, len2);
	critbit = MIN(len, ga_critbit(p0));

	if (p1 == p2 || critbit == len)
		r = GA_PCROSS_CUTSPLICE;
	else
		r = rand01();
	if (r < GA_PCROSS_ONEPOINT) {
		x = randinterval(critbit, len);
		ret1 = ga_cross_1p(c1, p1, x, p2);
		ret2 = ga_cross_1p(c2, p2, x, p1);
	} else if (r < GA_PCROSS_TWOPOINT) {
		x = randinterval(critbit, len);
		y = randinterval(x, len);
		ret1 = ga_cross_2p(c1, p1, x, y, p2);
		ret2 = ga_cross_2p(c2, p2, x, y, p1);
	} else { /* GA_PCROSS_CUTSPLICE */
		x = randinterval(critbit, len1);
		y = randinterval(critbit, len2);
		ret1 = ga_cut_splice(c1, p1, x, p2, y);
		ret2 = ga_cut_splice(c2, p2, y, p1, x);
	}
	if (ret1 || ret2)
		return -1;
	ga_bitflip(c2, critbit, bit_length(c2));
	ga_bitflip(c1, critbit, bit_length(c1));
	return 0;
}
