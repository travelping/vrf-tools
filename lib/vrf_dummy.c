/* vrf-tools
 *
 * Copyright (c) 2009, 2010, 2011 David Lamparter
 * please refer to the COPYING file for copying permission.
 */

#include "vrf.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

int vrf_socket(const char *vrf, int domain, int type, int protocol)
{
	return socket(domain, type, protocol);
}

int vrf_tap(const char *vrf)
{
	return open("/dev/net/tun", O_RDWR);
}

