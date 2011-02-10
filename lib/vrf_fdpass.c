#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#include "vrf.h"
#include "../src/vrf_ctlproto.h"

static int vrf_popfd(int sock)
{
	int32_t err;

	int fd;
	char buf[CMSG_SPACE(sizeof(fd))];

	struct msghdr msg;
	struct iovec iov;
	struct cmsghdr *cmsg;

	err = 0;
	iov.iov_base = &err;
	iov.iov_len = sizeof(err);

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (recvmsg(sock, &msg, 0) < 0)
		return -1;

	if (err != 0) {
		errno = err;
		return -1;
	}

	fd = -1;
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg;
			cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level != SOL_SOCKET
				|| cmsg->cmsg_type != SCM_RIGHTS)
			continue;
		if (cmsg->cmsg_len != CMSG_LEN(sizeof(fd))) {
			errno = EINVAL;
			return -1;
		}
		memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
	}
	if (fd == -1)
		errno = EINVAL;

	return fd;
}

static int vrf_connect(const char *vrf)
{
	struct sockaddr_un su;
	int sock;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
		return -1;

	su.sun_family = AF_UNIX;
	snprintf(su.sun_path, sizeof(su.sun_path), PATH_STATE"/%s/ctl", vrf);
	if (connect(sock, (struct sockaddr *)&su, sizeof(su))) {
		close(sock);
		return -1;
	}

	return sock;
}

static int vrf_sendcmd(int ctlfd, uint8_t cmd, const void *arg, size_t arglen)
{
	struct msghdr msg;
	struct iovec iov[2];
	ssize_t sent;

	iov[0].iov_base = &cmd;
	iov[0].iov_len = sizeof(cmd);
	iov[1].iov_base = (void *)arg;
	iov[1].iov_len = arglen;

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = iov;
	msg.msg_iovlen = arglen ? 2 : 1;
	sent = sendmsg(ctlfd, &msg, 0);

	if (sent < 0)
		return -1;
	return ((size_t)sent == sizeof(cmd) + arglen) ? 0 : -1;
}

static int vrf_do(const char *vrf, uint8_t cmd, const void *arg, size_t arglen)
{
	int ctlfd = vrf_connect(vrf), fd;

	if (ctlfd == -1)
		return -1;

	if (vrf_sendcmd(ctlfd, cmd, arg, arglen)) {
		close(ctlfd);
		return -1;
	}

	fd = vrf_popfd(ctlfd);
	close(ctlfd);
	return fd;
}

int vrf_socket(const char *vrf, int domain, int type, int protocol)
{
	struct {
		int32_t domain, type, protocol;
	} msg = { domain, type, protocol };

	if (!vrf)
		return socket(domain, type, protocol);
	return vrf_do(vrf, VRF_CMD_SOCKET, &msg, sizeof(msg));
}

int vrf_tap(const char *vrf)
{
	if (!vrf)
		return open("/dev/net/tun", O_RDWR);
	return vrf_do(vrf, VRF_CMD_TAP, NULL, 0);
}

