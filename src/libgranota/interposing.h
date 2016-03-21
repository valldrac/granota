#ifndef INTERPOSING_H
#define INTERPOSING_H

extern void *libc_dl_handle;
extern void *pthread_dl_handle;

int dl_handles_init();
void *loadsym(void *handler, const char *symbol);

#endif /* INTERPOSING_H */
