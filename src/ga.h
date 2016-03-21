#ifndef GA_H
#define GA_H

#include "critbit.h"
#include "summary.h"

#define GA_PCROSS_ONEPOINT	0.25
#define GA_PCROSS_TWOPOINT	0.50
#define GA_PCROSS_CUTSPLICE	1.00

#define GA_SIGMA_SCALING_COEFF	1.0	/* Higher values => less fitness
                                       pressure */

typedef int (*ga_map_callback_t)(struct bit *s, void *arg);

struct ga_context {
	struct critbit_tree phylo_tree;	/* phylogenetic tree */
};

struct ga_pool {
	struct bit **pop;		/* population */
	unsigned *selected;
	double *fitness;		/* array of scaled fitness values */
	double fitavg;			/* population average fitness */
	double sigma_factor;
	unsigned size;
	unsigned allocated;
	struct summary stats;
};

struct ga_context *ga_create();
struct ga_pool *ga_pool_new(unsigned size_hint);
void ga_pool_free(struct ga_pool *pool);
struct bit *ga_new_string();
void ga_free_string(struct bit *s);
int ga_has_children(struct bit *s);
size_t ga_critbit(struct bit *s);
void ga_destroy(struct ga_context *ctx);
int ga_insert(struct ga_context *ctx, struct bit *s, double score);
int ga_unique(struct ga_context *ctx, struct bit *s);
int ga_map(struct ga_context *ctx, struct bit *ancestor,
           ga_map_callback_t apply, void *arg);
int ga_populate(struct bit *s, struct ga_pool *pool);
void ga_fitness(struct ga_pool *pool);
void ga_select(struct ga_pool *pool);
void ga_genocide(struct ga_pool *pool);
int ga_mutate(struct ga_context *ctx, struct bit *p, struct bit *c);
int ga_crossover(struct ga_context *ctx, struct bit *p1, struct bit *p2,
                 struct bit *p0, struct bit *c1, struct bit *c2);

#endif /* GA_H */
