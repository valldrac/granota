#if HAVE_CONFIG_H
# include <config.h>
#endif

#define _GNU_SOURCE	/* get semtimedop */

#include "tracer.h"
#include "libgranota.h"
#include "child.h"
#include "sig.h"
#include "fd.h"
#include "coverage.h"
#include "edit.h"

#include <common.h>
#include <semop.h>
#include <input.h>
#include <files.h>
#include <session.h>
#include <target.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <err.h>
#include <errno.h>

pid_t tracer_pid;

pid_t child_pid;

static struct undo undo;

static struct memfile memfile;

static int tracer_exiting;

static int child_exiting;

static int
crashdbg(const char *pwd, int cpid, const char *infile)
{
	int pid, status, crash;
	const char *args[4];
	char pidstr[10];

	sprintf(pidstr, "%d", cpid);
	args[0] = PKGLIBEXECDIR "/crashdbg.sh";
	args[1] = pidstr;
	args[2] = infile;
	args[3] = NULL;

	pid = fork();
	if (pid < 0)
		err(4 ,"fork");
	if (pid == 0) {
		if (chdir(pwd) < 0)
			warn("%s", pwd);
		execvp(args[0], (char * const *) args);
		err(4, "%s", args[0]);
	}
	/*
	 * Parent process, wait for crashdbg to finish.
	 */ 
	while (waitpid(pid, &status, 0) != pid) {
		if (errno != EINTR)
			break;
	}
	crash = CRASH_UNKNOWN_ERROR;
	if (WIFEXITED(status)) {
		crash = WEXITSTATUS(status);
		if (crash >= CRASH_TYPES)
			errx(4, "%s: unexpected exit code %d", args[0], crash);
	}
	return crash;
}

static void
interrupt(int signum)
{
	tracer_exiting++;
}

static void
reapchild(int signum)
{
	int save_errno;
	int st;
	pid_t pid;

	save_errno = errno;
	pid = wait(&st);
	if (pid <= 0)
		return;
	if (pid == child_pid)
		child_exiting++;
	if (WIFSIGNALED(st) && WTERMSIG(st) == SIGINT)
		interrupt(SIGINT);
	if (WIFEXITED(st))
		debug("child %u exited (code=%u)", pid, WEXITSTATUS(st));
	else
		debug("child %u killed (signal=%u)", pid, WTERMSIG(st));
	errno = save_errno;
}

static void
dbgchild(int signum, siginfo_t *si, void *ucontext)
{
	int save_errno;
	int type;
	char tmp[PATH_MAX];

	save_errno = errno;
	type = crashdbg(shm.session->crashdir, si->si_pid,
	                get_input_path(tmp, shm.input));
	if (kill(si->si_pid, 0) == 0)
		kill(si->si_pid, SIGKILL);
	shm.session->crashes[type]++;
	errno = save_errno;
}

static void
setup_signals(struct sigaction oldact[3])
{
	struct sigaction sa;

	sa.sa_handler = child_crash;
	sa.sa_flags = SA_RESETHAND;
	sigemptyset(&sa.sa_mask);
	sig_fatal_set_act(&sa);

	/*
	 * Semop() is never automatically restarted, regardless of the setting
	 * of the SA_RESTART flag.
	 */
	sa.sa_handler = interrupt;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sig_fn.sigaction(SIGINT, &sa, &oldact[0]);

	sa.sa_handler = reapchild;
	sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sig_fn.sigaction(SIGCHLD, &sa, &oldact[1]);

	sa.sa_sigaction = dbgchild;
	sa.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGCHLD);
	sig_fn.sigaction(SIGUSR1, &sa, &oldact[2]);
}

static void
restore_signals(struct sigaction act[3])
{
	sig_fn.sigaction(SIGINT, &act[0], NULL);
	sig_fn.sigaction(SIGCHLD, &act[1], NULL);
	sig_fn.sigaction(SIGUSR1, &act[2], NULL);
}

static void
nullify_stdio(void)
{
	int fd;

	fd = fd_fn.open("/dev/null", O_RDWR);
	if (fd < 0)
		err(4, "/dev/null");

	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);

	if (fd > 2)
		fd_fn.close(fd);
}

static void
protect_shm(void)
{
	char *start, *end;

	start = (char *) shm.session;
	end = (char *) shm.fdbk;
	if (mprotect(start, end - start, PROT_READ) < 0)
		err(4, "mprotect");
}

static int
try_replace(struct atom *orig, struct atom *rep, struct magic_hash_set *set)
{
	int ret;

	ret = edit_replace(&memfile, &undo, orig->len, orig->str, rep->str);
	if (ret > 0)
		magic_hash_new(set, rep->hash);
	return ret;
}

static int
refine_input(struct feedback *fdbk, struct input *in)
{
	int x, y;
	
	for (;;) {
		if (magic_pop(&fdbk->stack, fdbk->depth, &x, &y) < 0) {
			if (--fdbk->depth < 0)
				break;
			edit_undo(&undo);
		} else {
			if (memfile.addr == NULL
			    && memfile_map(&memfile, in->critbit / CHAR_BIT,
			                   files_fd(&in->files, in->fileno)) < 0)
				break;
			if (try_replace(atom_member(&fdbk->magics, x),
				            atom_member(&fdbk->magics, y),
				            &fdbk->history) > 0) {
				fdbk->depth++;
				in->tries[in->fileno]++;
				return 0;
			}
		}
	}
	return -1;
}

static void
clear_feedback(struct feedback *fdbk)
{
	fdbk->depth = 0;
	if (memfile.addr) {
		edit_finish(&undo);
		memfile_unmap(&memfile);
	}
	magic_empty(&shm.fdbk->stack);
	atom_empty(&shm.fdbk->magics);
}

static void
next_input(struct input *in, double score)
{
	in->score[in->fileno] = score;
	in->fileno++;
	in->fileno %= in->files.num;
	in->tries[in->fileno] = 0;
}

static int
trace_child(int semid, int tmout)
{
	struct timespec ts, *timeout;
	int ret;

	if (tmout) {
		ts.tv_sec = tmout / 1000;
		ts.tv_nsec = (tmout % 1000) * 1000000UL;
		timeout = &ts;
	} else {
		timeout = NULL;
	}
	do {
		ret = sem_test_child(semid, timeout);
		if (ret == 0 || errno == EINTR)
			break;
		if (errno != EAGAIN)
			err(4, "sem_test_child");
		warnx("time expired");
		kill(child_pid, SIGKILL);
	} while (!child_exiting);

	if (ret == -1) {
		if (sem_reset_tracer(semid) < 0)
			err(4, "sem_reset_tracer");
	}
	cvrg_collect();
	return ret;
}

static void
post_tracer(int semid)
{
	if (sem_post_tracer(semid) < 0)
		err(4, "sem_post_tracer");
}

static void
post_tracer_wait_input(int semid)
{
	if (sem_post_tracer_wait_input(semid) < 0)
		err(4, "sem_post_tracer_wait_input");
}

static void
post_result(int semid)
{
	if (sem_post_result(semid) < 0)
		err(4, "sem_post_result");
}

static void
tracer_main()
{
	int semid;
	double score;
	struct input *in;

	in = shm.input;
	semid = in->semid;

	trace_child(semid, 0);
	cvrg_score();

	while (!child_exiting) {
		if (shm.fdbk->depth)
			post_tracer(semid);
		else
			post_tracer_wait_input(semid);
		trace_child(semid, shm.session->tmout);
		score = cvrg_score();
		if (score > 0 || refine_input(shm.fdbk, in) < 0) {
			next_input(in, score);
			post_result(semid);
			clear_feedback(shm.fdbk);
		}
	}
	child_exiting = 0;
}

void
tracer_fork()
{
	struct sigaction saveact[3];
	struct rlimit rlas, rlcore;
	long aslimit;

	tracer_pid = getpid();

	if (!session_begin(shm.session, tracer_pid))
		errx(4, "trace session locked");

	rlas.rlim_cur = shm.session->aslimit * 1024UL * 1024UL;
	rlas.rlim_max = rlas.rlim_cur;
	rlcore.rlim_cur = 0;
	rlcore.rlim_max = rlcore.rlim_cur;

	if (files_open(&shm.input->files, O_RDWR | O_CLOEXEC) == -1)
		errx(4, "files_open");

	setup_signals(saveact);

	if (cvrg_init() < 0)
		errx(4, "cvrg_init");

	while (!tracer_exiting) {
		child_pid = fork();
		if (child_pid == 0) {
			restore_signals(saveact);
			if (shm.session->aslimit)
				setrlimit(RLIMIT_AS, &rlas);
			setrlimit(RLIMIT_CORE, &rlcore);
			if (shm.session->quiet)
				nullify_stdio();
			if (memfile.addr)
				memfile_unmap(&memfile);
			protect_shm();
			return;
		}
		tracer_main();
	}

	files_close(&shm.input->files);
	session_end(shm.session);

	_exit(0);
}
