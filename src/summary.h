#ifndef SUMMARY_H
#define SUMMARY_H

#include <math.h>

/*
 * Compute the mean, sample variance, and standard deviation of a stream of data.
 *
 * Ref: Knuth TAOCP vol 2, 3rd edition, page 232.
 */

struct summary {
	unsigned long n;
	double mean;
	double m2;
};

static inline void
sum_clear(struct summary *sum)
{
	sum->n = 0;
	sum->mean = 0;
	sum->m2 = 0;
}

static inline void
sum_add(struct summary *sum, double x)
{
	double mean;

	mean = sum->mean;
	sum->n++;
	sum->mean = mean + (x - mean) / sum->n;
	sum->m2 += (x - mean) * (x - sum->mean);
}

static inline void
sum_sub(struct summary *sum, double x)
{
	unsigned long n;
	double mean;
	
	mean = sum->mean;
	n = sum->n--;
	sum->mean = (n * mean - x) / sum->n;
	sum->m2 -= (x - mean) * (x - sum->mean); 
}

static inline double __attribute__ ((const))
sum_mean(struct summary *sum)
{
	return sum->mean;
}

static inline double __attribute__ ((const))
sum_var(struct summary *sum)
{
	return sum->m2 / (sum->n - 1);
}

static inline double __attribute__ ((const))
sum_std(struct summary *sum)
{
	return sqrt(sum_var(sum));
}

#endif /* SUMMARY_H */
