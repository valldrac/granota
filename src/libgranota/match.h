#ifndef MATCH_H
#define MATCH_H

#include <target.h>

int match_tgt_file(const struct target *tgt, const char *path);
int match_tgt_socket(const struct target *tgt, int type,
                     union sockaddr_u *addr, int passive);

#endif /* MATCH_H */
