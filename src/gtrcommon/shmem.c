#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "shmem.h"
#include "feedback.h"
#include "input.h"
#include "session.h"
#include "target.h"

#include <common.h>

#include <afl/config.h>

#include <sys/types.h>
#include <sys/shm.h>

#define PROJ_ID		'G'

struct shm_hdr {
	unsigned char cvrg[MAP_SIZE];
	struct session sess ALIGNED(4096);
	struct target tgt;
	struct input in;
	struct feedback fdbk ALIGNED(4096);	
};

static void
shm_init(struct shm_desc *shm, struct shm_hdr *hdr, size_t size)
{
	shm->coverage = hdr->cvrg;
	shm->session = &hdr->sess;
	shm->target = &hdr->tgt;
	shm->input = &hdr->in;
	shm->fdbk = &hdr->fdbk;
	shm->segsz = size;
}

static int
shm_get(const char *name, struct shm_desc *shm, size_t size, int shmflgs)
{
	key_t key;
	int shmid;
	void *p;

	key = ftok(name, PROJ_ID);
	if (unlikely(key == -1))
		return -1;

	shmid = shmget(key, size, shmflgs);
	if (unlikely(shmid == -1))
		return -1;

	p = shmat(shmid, 0, 0);
	if (unlikely(p == (void *) -1))
		return -1;

	shm_init(shm, p, size);
	return shmid;
}

int
create_shm(const char *name, struct shm_desc *shm)
{
	return shm_get(name, shm, sizeof(struct shm_hdr), IPC_CREAT | IPC_EXCL | 0666);
}

int
attach_shm(const char *name, struct shm_desc *shm)
{
	return shm_get(name, shm, sizeof(struct shm_hdr), 0);
}

int
destroy_shm(int shmid)
{
	return shmctl(shmid, IPC_RMID, 0);
}
