#ifndef SHMEM_H
#define SHMEM_H

#include <stddef.h>
#include <stdint.h>

typedef uintptr_t offset_ptr_t;

#define SHM_OFFSET_TO_LOCAL(ptr, offset, base) \
	ptr = (typeof(ptr)) ((char *) base + offset)

#define SHM_LOCAL_TO_OFFSET(offset, ptr, base) \
	offset = (offset_ptr_t) ((char *) ptr - (char *) base)

struct session;
struct target;
struct input;
struct feedback;

struct shm_desc {
	unsigned char *coverage;
	struct session *session;
	struct target *target;
	struct input *input;
	struct feedback *fdbk;
	size_t segsz;
};

int create_shm(const char *name, struct shm_desc *shm);
int attach_shm(const char *name, struct shm_desc *shm);
int destroy_shm(int shmid);

#endif /* SHMEM_H */
