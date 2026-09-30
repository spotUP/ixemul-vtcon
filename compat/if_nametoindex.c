/* if_nametoindex (RFC 3493): ixemul has no interface index table, so no
 * name has an index -- 0, "no such interface", as the RFC says for an
 * unknown name. (libevent parses "fe80::1%eth0" scopes with it) */
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

unsigned int
if_nametoindex(const char *name)
{
	(void)name;
	return 0;
}
