#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "match.h"
#include "str.h"

static int
inet_cmp_addr(const struct sockaddr_in *si1, const struct sockaddr_in *si2)
{
	return str_fn.memcmp(si1, si2, sizeof(struct sockaddr_in));
}

static int
inet6_cmp_addr(const struct sockaddr_in6 *si1, const struct sockaddr_in6 *si2)
{
	return str_fn.memcmp(si1, si2, sizeof(struct sockaddr_in6));
}

static int
match_addr(const union sockaddr_u *s1, const union sockaddr_u *s2)
{
	if (s1->sa.sa_family != s1->sa.sa_family)
		return 0;

	switch (s1->sa.sa_family) {
	case AF_INET:
		return inet_cmp_addr(&s1->si, &s2->si) == 0;
	case AF_INET6:
		return inet6_cmp_addr(&s1->si6, &s2->si6) == 0;
	default:
		return 0;
        }
}

static int
match_pathname(const char *s1, const char *s2)
{
	return str_fn.strcmp(s1, s2) == 0;
}

int
match_tgt_file(const struct target *tgt, const char *path)
{
	return tgt && (tgt->type == TGT_FILE)
	       && match_pathname(path, tgt->u.file.pathname);
}

int
match_tgt_socket(const struct target *tgt, int type, union sockaddr_u *addr, int passive)
{
	return tgt && (tgt->type == TGT_SOCKET)
	       && (tgt->u.sock.passive == passive)
	       && (tgt->u.sock.type == type)
               && match_addr(&tgt->u.sock.dst, addr);
}
