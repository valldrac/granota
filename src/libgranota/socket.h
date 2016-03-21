#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>

typedef int (*accept_fn_t)(int sockfd, struct sockaddr *addr,
                           socklen_t *addrlen);
typedef ssize_t (*send_fn_t)(int sockfd, const void *buf, size_t len, int flags);
typedef ssize_t (*recv_fn_t)(int sockfd, void *buf, size_t len, int flags);

struct socket_lib_fn {
	accept_fn_t accept;
	send_fn_t send;
	recv_fn_t recv;
};

extern struct socket_lib_fn socket_fn;

void socket_lib_init();

#endif /* SOCKET_H */
