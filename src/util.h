#ifndef UTIL_H
#define UTIL_H

#include "nulist.h"

#include <time.h>

int listdir(int dirfd, struct nlist *list);
void sigcatch(int signum, void (*handler)(int), int restart);
void hexprint(const char *buf, size_t len, size_t offset, size_t highlight);
struct timespec difftimespec(struct timespec start, struct timespec end);

#endif /* UTIL_H */
