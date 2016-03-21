#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "util.h"
#include "term.h"
#include "common.h"
#include "strerr.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

int
listdir(int dirfd, struct nlist *list)
{
	struct dirent *d;
	DIR *dirp;
	char *path;
	struct stat st;

	dirp = fdopendir(dirfd);
	if (dirp == NULL) {
		strerr_warnsys("fdopendir error");
		return -1;
	}

	while ((d = readdir(dirp)) != NULL) {
		path = d->d_name;
		if (path[0] == '.')
			continue;
		if (fstatat(dirfd, path, &st, 0) < 0) {
			strerr_warnsys("%s", path);
			continue;
		}
		if (!S_ISREG(st.st_mode)) {
			strerr_warn("%s: is not a regular file", path);
			continue;
		}
		if (nlist_add_copy(list, path) < 0)
			return -1;
	}
	closedir(dirp);
	return 0;
}

void
sigcatch(int signum, void (*handler)(int), int restart)
{
	struct sigaction action;

	sigemptyset(&action.sa_mask);
	action.sa_flags = restart ? SA_RESTART : 0;
	action.sa_handler = handler;
	sigaction(signum, &action, NULL);
}

/*
 * From tcpdump, dump the buffer in emacs-hexl format 
 */
void
hexprint(const char *buf, size_t len, size_t offset, size_t highlight)
{
	size_t i, j, jm;
	int c;
	unsigned char n;

	for (i = 0; i < len; i += 0x10) {
		printf("  %.4lx: ", i + offset);
		jm = len - i;
		jm = jm > 16 ? 16 : jm;

		for (j = 0; j < jm; j++) {
			n = (unsigned char) buf[i+j];
			if (i+j < highlight) {
				if ((j % 2) == 1)
					printf(TINVERSE "%.2x" TRESET " ", n);
				else
					printf(TINVERSE "%.2x" TRESET, n);
			} else {
				if ((j % 2) == 1)
					printf("%.2x ", n);
				else
					printf("%.2x", n);
			}
		}
		for (; j < 16; j++) {
			if ((j % 2) == 1)
				printf("   ");
			else
				printf("  ");
		}
		printf(" ");
		for (j = 0; j < jm; j++) {
			c = buf[i+j];
			c = isprint(c) ? c : '.';
			if (i+j < highlight)
				printf(TINVERSE "%c" TRESET, c);
			else
				printf("%c", c);

		}
		printf("\n");
	}
}

struct timespec
difftimespec(struct timespec start, struct timespec end)
{
	struct timespec res;

	if (end.tv_nsec < start.tv_nsec) {
		res.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;        
		res.tv_sec = end.tv_sec - 1 - start.tv_sec;
	} else {
		res.tv_nsec = end.tv_nsec - start.tv_nsec;        
		res.tv_sec = end.tv_sec - start.tv_sec;
	}
	return res;
}
