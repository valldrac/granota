#ifndef SIG_H
#define SIG_H

#include <signal.h>

typedef void (*sighandler_t)(int);

typedef sighandler_t (*signal_fn_t)(int signum, sighandler_t handler);
typedef int (*sigaction_fn_t)(int signum, const struct sigaction *act,
                              struct sigaction *oldact);
typedef int (*sigprocmask_fn_t)(int how, const sigset_t *set,
                                sigset_t *oldset);
typedef int (*pthread_sigmask_fn_t)(int how, const sigset_t *set,
                                    sigset_t *oldset);

struct sig_lib_fn {
	signal_fn_t signal;
	sigaction_fn_t sigaction;
	sigprocmask_fn_t sigprocmask;
	pthread_sigmask_fn_t pthread_sigmask;
};

extern struct sig_lib_fn sig_fn;

void sig_lib_init();
void sig_fatal_set_act(const struct sigaction *act);

#endif /* SIG_H */
