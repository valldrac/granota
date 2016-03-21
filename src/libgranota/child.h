#ifndef CHILD_H
#define CHILD_H

#include <feedback.h>

void child_crash(int signum);
void wait_fuzzer(int semid);
void probe_memcmp(struct feedback *fdbk, const void *s1, const void *s2,
                  size_t len);

#endif /* CHILD_H */
