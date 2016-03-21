#ifndef STREAM_H
#define STREAM_H

#include <stdio.h>

typedef FILE *(*fopen_fn_t)(const char *path, const char *mode);
typedef int (*fclose_fn_t)(FILE *fp);

struct stream_lib_fn {
        fopen_fn_t fopen;
	fclose_fn_t fclose;
};

extern struct stream_lib_fn stream_fn;

void stream_lib_init();

#endif /* STREAM_H */
