#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "fuzz.h"
#include "term.h"
#include "util.h"
#include "common.h"
#include "progname.h"
#include "strerr.h"

#include <target.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define REFRESH 2

time_t program_start, last_refresh;
const char *exe;
struct fuzz_ctx *fuzz;
struct fuzz_info info;
struct target target;
unsigned long fuzz_evals;
int interval = REFRESH;

static void
stop(int signum)
{
	printf("\n+++ Caught Signal %s +++\n", strsignal(signum));
	/* Restore default signal handler */
	sigcatch(signum, SIG_DFL, 0);
	fuzz_stop(fuzz);
}

static void
refresh()
{
	time_t now, elapsed;
	int i;
	size_t len;
	struct rusage ru;
	unsigned long evals;

	getrusage(RUSAGE_SELF, &ru);

	now = time(NULL);
	elapsed = difftime(now, program_start);

	printf(TCLEAR
	       "   _oo__      %s %s up %u+%02u:%02u:%02u\n"
	       "  _\\-   \\__   Usage: %um%u.%02us user, %um%u.%02us sys, %lu vctxsw\n"
	       " _\\-)--)-,/_  Mem: %luk used, %lu pagefts\n"
	       "\n",
	       PACKAGE_NAME, PACKAGE_VERSION,
	       elapsed / 86400,
	       (elapsed % 86400) / 3600,
	       (elapsed % 3600) / 60,
	       elapsed % 60,
	       (ru.ru_utime.tv_sec / 60),
	       (ru.ru_utime.tv_sec % 60),
	       (ru.ru_utime.tv_usec / 10000),
	       (ru.ru_stime.tv_sec / 60),
	       (ru.ru_stime.tv_sec % 60),
	       (ru.ru_stime.tv_usec / 10000),
	       ru.ru_nvcsw, ru.ru_maxrss, ru.ru_minflt + ru.ru_majflt);

	fuzz_read_info(fuzz, &info);

	printf("Session: ");
	if (info.tpid && kill(info.tpid, 0) == 0)
		printf("R (running), %u tpid", info.tpid);
	else
		printf("S (waiting)");

	for (i = FUZZ_STAGES, evals = 0; --i >= 0; )
		evals += info.evals[i] + info.refs[i]; 

	if (now - last_refresh)
		printf(", %lu evals/s\n", (evals - fuzz_evals) / (now - last_refresh));
	else
		printf("\n");

	fuzz_evals = evals;
	last_refresh = now;

	printf("Crash: %u exploitable, %u probably, %u probably-not\n"
	       "       %u seen, %u unknown, %u error\n"
	       "Stage: #   cycles  evaluations    nodes   refines   collns\n",
	      info.crashes[CRASH_EXPLOITABLE],
	      info.crashes[CRASH_PROBABLY],
	      info.crashes[CRASH_PROBABLY_NOT],
	      info.crashes[CRASH_ALREADY_SEEN],
	      info.crashes[CRASH_UNKNOWN],
	      info.crashes[CRASH_UNKNOWN_ERROR]);

	for (i = FUZZ_STAGES, evals = 0; --i >= 0; ) {
		printf("    %s %u %8u %12u %8u %9u %8u\n",
		       (info.stgnum == i) ? "=>" : "  ", i,
		       info.cycles[i],
		       info.evals[i],
		       info.scores[i],
		       info.refs[i],
		       info.colls[i]);
	}
	if (info.pivotstr) {
		len = bit_buf_len(info.pivotstr);
		printf("Pivot: %lu/%lu seq, %lu children, ",
		       info.pivot, info.nodecnt, info.poolsize);
		if (info.pivottime)
			printf("%0.10f score (in %0.2f secs)\n",
			       info.pivotscore, info.pivottime);
		else
			printf("calculating...\n");
		printf("       %lu bytes, %lu/%lu critbit\n", 
		       len, info.pivotcritbit, bit_length(info.pivotstr));
		if (len > 0x10 * 8)
			len = 0x10 * 8;
		hexprint(bit_buf_get(info.pivotstr), len, 0,
		         info.pivotcritbit / CHAR_BIT);
	}
}

static void
usage(int status)
{
	if (status) {
		fprintf(stderr, "Usage: %s [OPTION]... EXE-PATH\n", program_name);
		fprintf(stderr, "Try '%s -h' for help.\n", program_name);
		exit(status);
	} else {
		printf("%s %s built " __DATE__ " " __TIME__"\n"
		       "\n",
		       PACKAGE_NAME, PACKAGE_VERSION);
		printf( "Usage: %s [OPTION]... EXE-PATH\n"
		       "\n"
		       "Target Selection:\n"
		       "  -f FILE       path to target FILE\n"
		       "\n"
		       "Directories:\n"
		       "  -i DIR        input DIR for test cases\n"
		       "  -o DIR        output DIR for test cases\n"
		       "  -a DIR        crash DIR for captured crashes\n"
		       "  -e DIR        temporary DIR (%s)\n"
		       "\n"
		       "Execution Control:\n"
		       "  -t MSEC       timeout for each input (%u MSEC)\n"
		       "  -m MB         memory limit for children process (%u MB)\n"
		       "  -q            null-ify children stdin, stdout, stderr\n"
		       "\n"
		       "Fuzzing Options:\n"
		       "  -l BYTES      maximum size for each input file (%u BYTES)\n"
		       "  -n FILES      backlog of the input queue (%u FILES)\n"
		       "  -s SECONDS    stage duration (%u SECONDS)\n"
		       "\n",
		       program_name,
		       FUZZ_TMPDIR,
		       FUZZ_TIMEOUT,
		       FUZZ_ASLIMIT,
		       FUZZ_MAXSIZE,
		       FUZZ_BACKLOG,
		       FUZZ_INTERVAL);
	}
	exit(status);
}

int
main(int argc, char *argv[])
{
	const char *progname;
	extern char *optarg;
	extern int optind, optopt;
	int opt, c;

	if ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	set_program_name(progname);

	sigcatch(SIGINT, stop, 1);
	sigcatch(SIGHUP, stop, 1);
	sigcatch(SIGQUIT, stop, 1);
	sigcatch(SIGTERM, stop, 1);
	sigcatch(SIGPIPE, stop, 1);
	sigcatch(SIGCHLD, stop, 1);
	sigcatch(SIGUSR1, stop, 1);

	fuzz = fuzz_create();
	if (fuzz == NULL)
		return 1;

	while ((opt = getopt(argc, argv, "a:e:f:i:m:n:l:o:t:s:qh")) != -1) {
		switch (opt) {
		case 'a':
			fuzz->crashdir = optarg;
			break;
		case 'e':
			fuzz->tmpdir = optarg;
			break;
		case 'f':
			target.type = TGT_FILE;
			strncpy(target.u.file.pathname, optarg, PATH_MAX);
			break;
		case 'i':
			fuzz->indir = optarg;
			break;
		case 'm':
			fuzz->aslimit = atoi(optarg);
			break;
		case 'n':
			fuzz->backlog = atoi(optarg);
			break;
		case 'l':
			fuzz->maxsize = atoi(optarg);
			break;
		case 'o':
			fuzz->outdir = optarg;
			break;
		case 't':
			fuzz->tmout = atoi(optarg);
			break;
		case 's':
			fuzz->interval = atoi(optarg);
			break;
		case 'q':
			fuzz->quiet++;
			break;
		case 'h':
			usage(0);
		default:
			usage(2);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1 || target.type == TGT_NONE)
		usage(2);

	exe = argv[0];
	fuzz->exe = argv[0];

	if (fuzz_setup(fuzz, &target) < 0) {
		fuzz_destroy(fuzz);
		return 1;
	}
	printf("+++ Initialization Complete +++\n"
	       "You can now run the program:\n"
	       "LD_PRELOAD=" PKGLIBDIR "/libgranota.so %s ...\n",
	       fuzz->exe);

	last_refresh = time(&program_start);

	if (fuzz_start(fuzz, refresh))
		printf("+++ Aborted Fuzzing Session +++\n");

	printf("+++ Cleaning Up +++\n");
	fuzz_destroy(fuzz);
	return 0;
}
