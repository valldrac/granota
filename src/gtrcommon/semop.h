#ifndef SEMOP_H
#define SEMOP_H

#include <common.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

/*
 * Semaphores:
 *
 * 0 - result ready
 * 1 - input ready
 * 2 - tracer ready
 * 3 - child busy
 * 4 - target busy
 */

static inline int
create_sem()
{
	int semid;
	union semun s_u;
	short array[5] = { 0, 0, 0, 1, 0 };

	semid = semget(IPC_PRIVATE, 5, 0666);
	if (unlikely(semid != -1)) {
		s_u.array = array;
		semctl(semid, 0, SETALL, s_u);
	}
	return semid;
}

static inline int
destroy_sem(int semid)
{
	return semctl(semid, 0, IPC_RMID);
}

static inline int
semop_restart(int semid, struct sembuf *sops, unsigned nsops)
{
	int ret;

	do {
		ret = semop(semid, sops, nsops);
	} while (unlikely(ret == -1 && errno == EINTR));
	return ret;
}

static inline int
sem_reset_tracer(int semid)
{
	union semun s_u;

	s_u.val = 0;
	return semctl(semid, 2, SETVAL, s_u);
}

static inline int
sem_wait_result(int semid)
{
	struct sembuf sops[1];

	sops[0].sem_num = 0;
	sops[0].sem_op = -1;
	sops[0].sem_flg = 0;

	return semop(semid, sops, 1);
}

static inline int
sem_post_result(int semid)
{
	struct sembuf sops[1];

	sops[0].sem_num = 0;
	sops[0].sem_op = 1;
	sops[0].sem_flg = 0;

	return semop_restart(semid, sops, 1);
}

static inline int
sem_post_tracer(int semid)
{
	struct sembuf sops[1];

	sops[0].sem_num = 2;	
	sops[0].sem_op = 1;
	sops[0].sem_flg = 0;

	return semop_restart(semid, sops, 1);
}

static inline int
sem_post_tracer_wait_input(int semid)
{
	struct sembuf sops[2];

	/* sem_post_tracer */
	sops[0].sem_num = 2;	
	sops[0].sem_op = 1;
	sops[0].sem_flg = 0;

	/* sem_wait_input */
	sops[1].sem_num = 1;
	sops[1].sem_op = -1;
	sops[1].sem_flg = 0;

	return semop_restart(semid, sops, 2);
}

static inline int
sem_post_input(int semid)
{
	struct sembuf sops[1];

	sops[0].sem_num = 1;
	sops[0].sem_op = 1;
	sops[0].sem_flg = 0;

	return semop(semid, sops, 1);
}

static inline int
sem_test_child(int semid, struct timespec *timeout)
{
	struct sembuf sops[1];

	sops[0].sem_num = 3;
	sops[0].sem_op = 0;
	sops[0].sem_flg = 0;

	return semtimedop(semid, sops, 1, timeout);
}

static inline int
sem_wait_child_test_target(int semid)
{
	struct sembuf sops[2];

	/* wait_child */
	sops[0].sem_num = 3;
	sops[0].sem_op = -1;
	sops[0].sem_flg = SEM_UNDO;

	/* test_target */
	sops[1].sem_num = 4;
	sops[1].sem_op = 0;
	sops[1].sem_flg = 0;

	return semop_restart(semid, sops, 2);
}

static inline int
sem_wait_tracer_post_child(int semid)
{
	struct sembuf sops[2];

	/* wait_tracer */;
	sops[0].sem_num = 2;
	sops[0].sem_op = -1;
	sops[0].sem_flg = 0;

	/* post_child */
	sops[1].sem_num = 3;
	sops[1].sem_op = 1;
	sops[1].sem_flg = SEM_UNDO;

	return semop_restart(semid, sops, 2);
}

#endif /* SEMOP_H */
