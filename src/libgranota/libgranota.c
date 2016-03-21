#if HAVE_CONFIG_H
# include <config.h>
#endif

#define _GNU_SOURCE	/* get assert_perror */

#include "libgranota.h"
#include "interposing.h"
#include "sig.h"
#include "fd.h"
#include "socket.h"
#include "stream.h"
#include "str.h"

#include <common.h>
#include <self.h>

#include <afl/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <err.h>

struct shm_desc shm;

int shmid;

EXPORT INIT(0)
void
libgranota_init()
{
	char exe[PATH_MAX];
	size_t path_len;
	char shm_str[20];

	if (dl_handles_init())
		return;

	fd_lib_init();
	sig_lib_init();
	socket_lib_init();
	stream_lib_init();
	str_lib_init();

	path_len = sizeof(exe);
	if (self_path(exe, &path_len) < 0)
		err(4, "could not get executable path");

	shmid = attach_shm(exe, &shm);
	if (shmid != -1) {
		sprintf(shm_str, "%d", shmid);
		setenv(SHM_ENV_VAR, shm_str, 1);
	} else {
		unsetenv(SHM_ENV_VAR);
	}
}

EXPORT FINI(101)
void
libgranota_fini()
{
}

