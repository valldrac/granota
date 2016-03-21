#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "random.h"

#include <math.h>

/*
 * Random number variables for SWB.
 */
int seed;
int i96, j96;
float u[97], c, cd, cm;

/*
 * Flip a biased coin (p = 0.5 is a fair coin).
 */ 
int
flipcoin(float p)
{
	return ((p == 1) || rand01() < p) ? 1 : 0;
}

/*
 * Returns a uniform random integer on the specified interval.
 */
long
randinterval(long start, long end)
{
	return (double) rand01() * (end - start + 1) + start;
}

/*
 * Returns a uniform random number on the interval.
 */
double
randuniform(double start, double end)
{
	return (end - start) * rand01() + start;
}

/*
 * Returns an approximation to a Gaussian random number.
 */
double
randnormal(double mean, double sigma)
{
	int i;
	double sum = 0.;

	for (i = 11; i >= 0; i--)
		sum += rand01();

	return (sum - 6.0) * sigma + mean;
}

/*
 * Returns a Poisson-distributed number.
 */
int
randpoiss(double lambda)
{
	int k = 0;
	double l, p;

	l = exp(-lambda);
	p = rand01();

	while (p > l) {
		p *= rand01();
		k++;
	} 
	return k;
}

/*
 * Subtract-with-Borrow Congruential Generator.
 *
 * Ref: G. Marsaglia, A. Zaman, A New Class of Random Number Generators.
 * Annals of Applied Probability vol 1 no 3 (1991), 462-480.
 */

/*
 * Set a seed for the random number generator.
 */
float
randseed(int newseed)
{
	int ij, kl, i, j, k, l, m, ii, jj;
	float s, t;

	/*
	 * Initialization variables.
	 */
	seed = newseed % 900000000;
	ij   = seed / 30082;
	kl   = seed - 30082 * ij;
	i    = ((ij/177) % 177) + 2;
	j    = ( ij      % 177) + 2;
	k    = ((kl/169) % 178) + 1;
	l    = ( kl      % 169);

	for (ii = 0; ii < 97; ii++) {
		s = 0.0;
		t = 0.5;
		for (jj = 0; jj < 24; jj++) {
			m = (((i * j) % 179) * k) % 179;
			i = j;
			j = k;
			k = m;
			l = ((53 * l) + 1 ) % 169;
			if (((l * m) % 64 ) >= 32)
				s += t;
				t *= .5;
			}
		u[ii] = s;
       	}

        c   = 362436.   / 16777216.;
        cd  = 7654321.  / 16777216.;
        cm  = 16777213. / 16777216.;
        i96 = 96;
        j96 = 32;

	return rand01();
}

/*
 * Returns a uniform random number on the interval [0,1)
 */
float
rand01()
{
	float uni;

	uni = u[i96] - u[j96];
	if (uni < 0.)
		uni += 1.0;
	u[i96] = uni;
	i96--;
	if (i96 < 0)
		i96 = 96;
	j96--;
	if (j96 < 0)
		j96 = 96;
	    c -= cd;
	if (c  < 0.)
		c += cm;
	uni -= c;
	if (uni < 0.)
		uni += 1.0;

	return uni;
}
