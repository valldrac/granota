#ifndef TARGET_H
#define TARGET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <limits.h>

enum tgt_type {
	TGT_NONE = 0,
	TGT_SOCKET = 1,
	TGT_FILE = 2,
	TGT_ANY = TGT_SOCKET | TGT_FILE,
};

union sockaddr_u {
        struct sockaddr sa;
        struct sockaddr_in si;
        struct sockaddr_in6 si6;
        struct sockaddr_un su;
        struct sockaddr_storage ss;
};

struct target_socket {
        int type;
        int passive;
        union sockaddr_u dst;
        union sockaddr_u src;
};

struct target_file {
	char pathname[PATH_MAX];
};

union target_u {
	struct target_socket sock;
	struct target_file file;
};

struct target {
	int type;
	union target_u u;
};

#endif /* TARGET_H */
