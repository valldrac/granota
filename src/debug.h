#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#define D_PRINTF(x...) fprintf(stderr, x)

#ifdef DEBUG
# define debug(x...) D_PRINTF(x)
# define trace(x...) do { \
	  D_PRINTF("[!] TRACE: in %s, at %s:%u: ", __FUNCTION__, __FILE__, __LINE__); \
	  D_PRINTF(x); \
	  D_PRINTF("\n"); \
	} while (0)
#else
# define debug(x...) ((void)0)
# define trace(x...) ((void)0)
#endif

#endif /* DEBUG_H */
