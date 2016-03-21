#ifndef RANDOM_H
#define RANDOM_H

#include <stddef.h>

/*
 * Shuffle an array of k elements.
 */
#define suffle(array, k)		\
	do {				\
	  typeof (array[0]) tmp_;	\
	  ssize_t i_ = k, j_;		\
	  for (--i_; i_ > 0; --i_) {	\
	    j_ = randinterval(0, i_);	\
	    tmp_ = array[i_];		\
	    array[i_] = array[j_];	\
	    array[j_] = tmp_;		\
	  }				\
	} while (0)

int flipcoin(float p);
long randinterval(long start, long end);
double randuniform(double start, double end);
double randnormal(double mean, double sigma);
int randpoiss(double lambda);
float randseed(int newseed);
float rand01();

#endif /* RANDOM_H */
