#include "strerr.h"
#include "progname.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* 
 * Print program_name prefix on stderr if program_name is not NULL 
 */ 
static void 
maybe_print_progname() 
{ 
        if (program_name) 
                fprintf(stderr, "%s: ", program_name); 
}

void
strerr_warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	maybe_print_progname();
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
}

void
strerr_warnsys(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	maybe_print_progname();
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ": ");
	perror(0);
	va_end(ap);
}

void
strerr_diesys(int status, const char *fmt, ...)
{
	va_list ap;
	static int exiting;

	if (exiting)
		exit(status);
	exiting++;

	va_start(ap, fmt);
	maybe_print_progname();
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ": ");
	perror(0);
	va_end(ap);
	exit(status);
}

void
strerr_die(int status, const char *fmt, ...)
{
	va_list ap;
	static int exiting;

	if (exiting)
		exit(status);
	exiting++;

	va_start(ap, fmt);
	maybe_print_progname();
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
	exit(status);
}

