#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "socket.h"
#include "match.h"
#include "interposing.h"
#include "fd.h"

struct socket_lib_fn socket_fn;

void
socket_lib_init()
{
	socket_fn.accept = loadsym(libc_dl_handle, "accept");
	socket_fn.send = loadsym(libc_dl_handle, "send");
	socket_fn.recv = loadsym(libc_dl_handle, "recv");
}


/*
 * Socket library function overrides.
 */

#if 0

EXPORT
int
accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	struct target *target;
	struct sockaddr_u listener;
	socklen_t lislen, optlen;
	int socktype, flags, pl, fd, ret;

	/*
	 * Retrieve the current address to which the listener is bound.
	 */
	lislen = sizeof(listener);
	ret = getsockname(sock, &listener.sa, &lislen);
	if (ret < 0)
		return -1;

	/*
	 * Ask for the socket type.
	 */
	optlen = sizeof(socktype);
	getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &socktype, &optlen);

	target = tgt_lookup_socket(shmem.tgt_begin, socktype, listener, 1);
	if (target) {
		fd = socket(target->u.sock.addr.sa.sa_family, socktype, 0);
		if (likely(fd >= 0)) {
			pl = tgt_get_payload(target);
			fd_install(fd, bits_open(shmem.bits[pl]), NULL);
		}
	} else {
		fd = socket_fn.accept(sockfd, addr, addrlen);
	}

	return fd;
}

#endif
