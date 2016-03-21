#ifndef TRACER_H
#define TRACER_H

#include <unistd.h>

extern pid_t tracer_pid;
extern pid_t child_pid;

void tracer_fork();

#endif /* TRACER_H */
