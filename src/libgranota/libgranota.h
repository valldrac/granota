#ifndef LIBGRANOTA_H
#define LIBGRANOTA_H

#include <shmem.h>

extern struct shm_desc shm;

void libgranota_init();
void libgranota_fini();

#endif /* LIBGRANOTA_H */
