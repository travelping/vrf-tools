#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "netns.h"

void safe_write(int fd, void *buf, size_t len)
{
	ssize_t written;
	char *ptr = buf;

	do {
		written = write(fd, ptr, len);
		if (written < 0) {
			if (errno == EAGAIN)
				continue;
			else
				break;
		}
		ptr += written;
		len -= written;
	} while (len > 0);

	if (written < 0)
		perror("write");
}

int safe_read(int fd, void *buf, size_t len)
{
	ssize_t nread;
	char *ptr = buf;

	do {
		nread = read(fd, ptr, len);
		if (nread < 0) {
			if (errno == EAGAIN)
				continue;
			else
				return -1;
		}
		ptr += nread;
		len -= nread;
	} while (len > 0);

	return 0;
}


