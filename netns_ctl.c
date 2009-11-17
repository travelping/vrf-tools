#define _GNU_SOURCE
#include "acconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

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

void ctl_run(int ctlsock)
{
	int cfd;

	do {
		errno = 0;
		while ((cfd = accept(ctlsock, NULL, 0)) >= 0) {
			close(cfd);
		}
	} while (errno == 0 || errno == EAGAIN || errno == EINTR);
}

