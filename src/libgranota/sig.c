#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "sig.h"
#include "interposing.h"

#include <common.h>

struct sig_lib_fn sig_fn;

static void *sighand[NSIG];

static int
sigfatal(int signum)
{
	switch (signum) {
	case SIGILL:
	case SIGABRT:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
		return 1;
	default:
		return 0;
	}
}

static struct sigaction
sigsacpy(const struct sigaction *act)
{
	return *act;
}

static sigset_t
sigsetcpy(const sigset_t *set)
{
	return *set;
}

static void
sigdelset_fatal(sigset_t *set)
{
	sigdelset(set, SIGILL);
	sigdelset(set, SIGABRT);
	sigdelset(set, SIGFPE);
	sigdelset(set, SIGSEGV);
	sigdelset(set, SIGBUS);
}

void
sig_lib_init()
{
	sig_fn.signal = loadsym(libc_dl_handle, "signal");
	sig_fn.sigaction = loadsym(libc_dl_handle, "sigaction");
	sig_fn.sigprocmask = loadsym(libc_dl_handle, "sigprocmask");
	if (pthread_dl_handle) {
		sig_fn.pthread_sigmask = loadsym(pthread_dl_handle,
		                                 "pthread_sigmask");
	}
}

void
sig_fatal_set_act(const struct sigaction *act)
{
	sig_fn.sigaction(SIGILL, act, NULL);
	sig_fn.sigaction(SIGABRT, act, NULL);
	sig_fn.sigaction(SIGFPE, act, NULL);
	sig_fn.sigaction(SIGSEGV, act, NULL);
	sig_fn.sigaction(SIGBUS, act, NULL);
}

/*
 * Signal library function overrides.
 */

EXPORT
sighandler_t
signal(int signum, sighandler_t handler)
{
	sighandler_t old;

	if (sigfatal(signum)) {
		old = sighand[signum];
		sighand[signum] = handler;
	} else {
		old = sig_fn.signal(signum, handler);
	}
	return old;
}

EXPORT
int
sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
	void *old;
	struct sigaction newact;
	int isfatal, ret;

	isfatal = sigfatal(signum);
	if (isfatal) {
		old = sighand[signum];
		if (act) {
			if (act->sa_flags & SA_SIGINFO)
				sighand[signum] = act->sa_sigaction;
			else
				sighand[signum] = act->sa_handler;
			act = NULL;	/* force our signal handler */
		}
	}
	if (act) {
		newact = sigsacpy(act);
		sigdelset_fatal(&newact.sa_mask);
		act = &newact;
	}

	ret = sig_fn.sigaction(signum, act, oldact);

	if (isfatal && oldact) {
		if (oldact->sa_flags & SA_SIGINFO)
			oldact->sa_sigaction = old;
		else
			oldact->sa_handler = old;
	}
	return ret;
}

EXPORT
int
sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
	sigset_t newset;

	if (set) {
		newset = sigsetcpy(set);
		sigdelset_fatal(&newset);
		set = &newset;
	}
	return sig_fn.sigprocmask(how, set, oldset);
}

EXPORT
int
pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset)
{
	sigset_t newset;

	if (set) {
		newset = sigsetcpy(set);
		sigdelset_fatal(&newset);
		set = &newset;
	}
	return sig_fn.pthread_sigmask(how, set, oldset);
}
