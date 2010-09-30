#define _GNU_SOURCE
#include "acconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

#include "netns_ctlproto.h"
#include "netns.h"

int ctl_open(const char *ns)
{
	struct sockaddr_un addr;
	int sock;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1) {
		fprintf(stderr, "error creating unix control socket: %s\n",
				strerror(errno));
		return -1;
	}

	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), PATH_STATE"/%s/ctl", ns);
	unlink(addr.sun_path);
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
		fprintf(stderr, "error binding unix control socket to '%s': %s\n",
				addr.sun_path, strerror(errno));
		close(sock);
		return -1;
	}

	if (listen(sock, 5)) {
		fprintf(stderr, "error listening on unix control socket: %s\n",
				strerror(errno));
		close(sock);
		return -1;
	}
	return sock;
}

static void ctl_pushfd(int sock, int fd)
{
	int32_t err;

	if (fd == -1) {
		err = errno;
		safe_write(sock, &err, sizeof(err));
	} else {
		struct msghdr msg;
		struct iovec iov;
		struct cmsghdr *cmsg;
		int *ptr;
		char buf[CMSG_SPACE(sizeof(int))];

		err = 0;
		iov.iov_base = &err;
		iov.iov_len = sizeof(err);

		memset(&msg, 0, sizeof(msg));

		msg.msg_control = buf;
		msg.msg_controllen = sizeof buf;
		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;

		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		ptr = (int *) CMSG_DATA(cmsg);
		*ptr = fd;

		msg.msg_controllen = cmsg->cmsg_len;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		sendmsg(sock, &msg, 0);

		close(fd);
	}
}

static void ctl_socket(int sock)
{
	int32_t af = -1, type = -1, proto = -1;
	int rsock;

	if (safe_read(sock, &af, sizeof(af))
		|| safe_read(sock, &type, sizeof(type))
		|| safe_read(sock, &proto, sizeof(proto)))
		return;

	rsock = socket(af, type, proto);
	ctl_pushfd(sock, rsock);
}

static void ctl_tap(int sock)
{
	int tap;

	tap = open("/dev/net/tun", O_RDWR);
	ctl_pushfd(sock, tap);
}

static void ctl_child(int sock)
{
	uint8_t cmd;
	ssize_t nread;

	while ((nread = read(sock, &cmd, 1)) == 0)
		;
	if (nread < 0)
		return;

	switch (cmd) {
	case NETNS_CMD_NOOP:
		safe_write(sock, &cmd, 1);
		return;
	case NETNS_CMD_SOCKET:
		ctl_socket(sock);
		return;
	case NETNS_CMD_TAP:
		ctl_tap(sock);
		return;
	}
}

void ctl_run(int ctlsock)
{
	int cfd;

	if (ctlsock < 0)
		return;
	do {
		errno = 0;
		while ((cfd = accept(ctlsock, NULL, 0)) >= 0) {
			pid_t pid = fork();
			if (pid < 0)
				close(cfd);
			if (pid == 0) {
				close(ctlsock);
				ctl_child(cfd);
				close(cfd);
				exit(0);
			}
			if (pid > 0)
				close(cfd);
		}
	} while (errno == 0 || errno == EAGAIN || errno == EINTR);
}

