#include "netns.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

int netns_socket(const char *netns, int domain, int type, int protocol)
{
	return socket(domain, type, protocol);
}

int netns_tap(const char *netns)
{
	return open("/dev/net/tun", O_RDWR);
}

