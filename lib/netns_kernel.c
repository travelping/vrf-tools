#define _GNU_SOURCE
#include "netns.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>

#ifdef __NR_setns
/* use defined value */
#elif defined(__x86_64__)
#define __NR_setns 300
#elif defined(__i386__)
#define __NR_setns 338
#elif defined(__arm__)
#define __NR_setns 366
#else
#error setns syscall number not known
#endif

static int sys_setns(unsigned what, int fd)
{
	return syscall(__NR_setns, what, fd);
}

static int enter_netns(const char *netns)
{
	int old, new;
	char name[PATH_MAX];

	if (!netns)
		return -2;

	if (isdigit(netns[0])) {
		/* TBA: input validation */
		snprintf(name, sizeof(name), "/proc/%s/ns/net", netns);
	} else {
		snprintf(name, sizeof(name), "%s/%s/ns", PATH_STATE, netns);
	}

	new = open(name, O_RDONLY);
	if (new == -1)
		return -1;

	old = open("/proc/self/ns/net", O_RDONLY);
	if (old == -1) {
		close(new);
		return -1;
	}

	sys_setns(0, new);
	close(new);
	return old;
}

static void leave_netns(int old)
{
	if (old != -2) {
		sys_setns(0, old);
		close(old);
	}
}

int netns_socket(const char *netns, int domain, int type, int protocol)
{
	int fd, old;

	old = enter_netns(netns);
	if (old == -1)
		return -1;

	fd = socket(domain, type, protocol);
	leave_netns(old);

	return fd;
}

int netns_tap(const char *netns)
{
	int fd, old;

	old = enter_netns(netns);
	if (old == -1)
		return -1;

	fd = open("/dev/net/tun", O_RDWR);
	leave_netns(old);

	return fd;
}

