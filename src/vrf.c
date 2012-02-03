/* vrf-tools
 *
 * Copyright (c) 2009, 2010, 2011 David Lamparter
 * please refer to the COPYING file for copying permission.
 */

#define _GNU_SOURCE
#include "acconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#ifdef HAVE_SCHED_H
#include <sched.h>
#endif
#ifdef HAVE_LINUX_UNISTD_H
#include <linux/unistd.h>
#endif

#ifndef PATH_CFG
#error define PATH_CFG
#endif
#ifndef PATH_STATE
#error define PATH_STATE
#endif

#include "vrf_app.h"

static int verbose = 0;

static int usage(int rv)
{
	FILE *fd = rv ? stderr : stdout;
	fprintf(fd, "\
Usage:	vrf -h\n\
	vrf [-v] <OPERATION> <NAMESPACE NAME>\n\n\
where OPERATION can be:\n\
	start		start the namespace\n\
	getpid		print the namespace pid if it is running\n\
\n\
");
	return rv;
}

static int launch_cmd(const char *exe, char * const *args)
{
	pid_t pid;
	int fd;
	int status;

	pid = fork();
	switch (pid) {
	case -1:
		perror("fork");
		return -1;
	case 0:
		for (fd = 3; fd < 10; fd++)
			close(fd);
		if (verbose >= 1)
			fprintf(stderr, "launching %s\n", exe);
		execv(exe, args);
		fprintf(stderr, "%s: %s\n", exe, strerror(errno));
		exit(1);
	default:
		if (waitpid(pid, &status, 0) < 0) {
			perror("waitpid");
			return 1;
		}
		return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
	}
}

static char pidpath[PATH_MAX];
static pid_t mainpid;

static void removepid()
{
	if (getpid() == mainpid)
		unlink(pidpath);
}

static int writepid(const char *vrf)
{
	char pid[32];
	int pidfd;
	struct flock lck;

	snprintf(pidpath, sizeof(pidpath), PATH_STATE"/%s/pid", vrf);
	pidfd = open(pidpath, O_RDWR | O_NONBLOCK | O_TRUNC | O_CREAT, 0644);
	if (pidfd < 0) {
		fprintf(stderr, "%s: %s\n", pidpath, strerror(errno));
		return 1;
	}

	mainpid = getpid();
	snprintf(pid, sizeof(pid), "%ld\n", (long)mainpid);
	safe_write(pidfd, pid, strlen(pid));

	lck.l_type = F_WRLCK;
	lck.l_whence = SEEK_SET;
	lck.l_start = 0;
	lck.l_len = strlen(pid);
	lck.l_pid = 0;
	if (fcntl(pidfd, F_SETLK, &lck) < 0) {
		fprintf(stderr, "fcntl(%s, F_SETLK): %s\n", pidpath,
				strerror(errno));
		close(pidfd);
		unlink(pidpath);
		return 1;
	}
	atexit(removepid);
	return 0;
}

static int do_start_child(const char *vrf, int notifier, int waiter)
{
	int ok = 0, rv, ctlsock;
	struct pollfd p;

	char exe[256], argv0[256], *args[] = { argv0, NULL };

	struct sigaction sa;
	int sig;
	sigset_t set;

	/* write pid first to catch errors */

	rv = writepid(vrf);
	if (rv) {
		safe_write(notifier, &rv, sizeof(rv));
		return 1;
	}
	ctlsock = ctl_open(vrf);

	if (unshare(CLONE_NEWNET | CLONE_NEWUTS) == -1) {
		perror("unshare");
		rv = 1;
		safe_write(notifier, &rv, sizeof(rv));
		return 1;
	}
	if (verbose >= 1)
		fprintf(stderr, "%s: pid %ld\n", vrf, (long)getpid());

	safe_write(notifier, &ok, sizeof(ok));

	/* master now runs /etc/vrf/(name)/assign */

	p.fd = waiter;
	p.events = POLLIN;
	if (poll(&p, 1, -1) <= 0) {
		perror("poll");
		return 1;
	}
	safe_read(waiter, &rv, sizeof(rv));
	if (rv)
		return 1;

	close(waiter);

	setsid();

	snprintf(argv0, sizeof(argv0), "vrf: %s", vrf);
	snprintf(exe, sizeof(exe), PATH_CFG "/%s/start", vrf);
	rv = launch_cmd(exe, args);
	if (rv) {
		fprintf(stderr, "%s exited with status %d\n", exe, rv);
		safe_write(notifier, &rv, sizeof(rv));
		return 1;
	}

	if (verbose >= 1)
		fprintf(stderr, "%s: up and running\n", vrf);

	safe_write(notifier, &ok, sizeof(ok));
	close(notifier);

	close(0);
	close(1);
	close(2);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_NOCLDWAIT;
	sigaction(SIGCHLD, &sa, NULL);

	ctl_run(ctlsock);

	/* hmm. control socket bailed, just wait forever... */
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	while (1)
		sigwait(&set, &sig);
}

static int do_start_parent(const char *vrf, int waiter, int notifier, int pid)
{
	int status = -1, ok = 0;
	char spid[10];
	struct pollfd p;

	char exe[256], argv0[256], *args[] = { argv0, NULL };

	snprintf(spid, sizeof(spid), "%d", pid);
	setenv("VRF_PID", spid, 1);
	setenv("VRF_NAME", vrf, 1);

	/* child does unshare() */

	p.fd = waiter;
	p.events = POLLIN;
	if (poll(&p, 1, -1) <= 0) {
		perror("poll");
		return 1;
	}
	safe_read(waiter, &status, sizeof(status));
	if (status) {
		fprintf(stderr, "child flagged an error, aborting.\n");
		return 1;
	}

	snprintf(argv0, sizeof(argv0), "vrf-assign: %s", vrf);
	snprintf(exe, sizeof(exe), PATH_CFG "/%s/assign", vrf);
	status = launch_cmd(exe, args);
	if (status) {
		fprintf(stderr, "%s exited with status %d\n", exe, status);
		safe_write(notifier, &status, sizeof(status));
		return 1;
	}

	safe_write(notifier, &ok, sizeof(ok));

	if (poll(&p, 1, -1) <= 0) {
		perror("poll");
		return 1;
	}
	safe_read(waiter, &status, sizeof(status));
	if (status) {
		fprintf(stderr, "child flagged an error, aborting.\n");
		return 1;
	}
	return 0;
}

static int do_start(const char *vrf)
{
	pid_t pid;
	int sync1[2], sync2[2];

	if (pipe(sync1) < 0 || pipe(sync2) < 0) {
		perror("pipe");
		return 1;
	}

	pid = fork();
	switch (pid) {
	case -1:
		perror("fork");
		return 1;
	case 0:
		close(sync1[0]);
		close(sync2[1]);
		return do_start_child(vrf, sync1[1], sync2[0]);
	default:
		close(sync1[1]);
		close(sync2[0]);
		return do_start_parent(vrf, sync1[0], sync2[1], pid);
	}
}

static int do_getpid(const char *vrf)
{
	int pidfd;
	struct flock lck;

	snprintf(pidpath, sizeof(pidpath), PATH_STATE"/%s/pid", vrf);
	pidfd = open(pidpath, O_RDWR | O_NONBLOCK);
	if (pidfd < 0) {
		fprintf(stderr, "%s: %s\n", pidpath, strerror(errno));
		return 1;
	}

	lck.l_type = F_WRLCK;
	lck.l_whence = SEEK_SET;
	lck.l_start = 0;
	lck.l_len = 1;
	lck.l_pid = 0;
	if (fcntl(pidfd, F_GETLK, &lck) < 0) {
		fprintf(stderr, "fcntl(%s, F_GETLK): %s\n", pidpath,
				strerror(errno));
		return 1;
	}

	if (lck.l_type == F_WRLCK) {
		printf("%d", lck.l_pid);
		return 0;
	}
	fprintf(stderr, "%s: not running (pid file not locked)\n", vrf);
	return 2;
}

int main(int argc, char * const argv[])
{
	int c;
	const char *cmd, *vrf;
	char vrfpath[PATH_MAX];
	struct stat vrf_stat;

	do {
		c = getopt(argc, argv, "hv");
		switch (c) {
		case -1:
			break;
		case 'h':
			return usage(0);
		case 'v':
			verbose++;
			break;
		default:
			return usage(1);
		}
	} while (c != -1);

	if (optind != argc - 2) {
		fprintf(stderr, "invalid number of arguments.\n");
		return 1;
	}
	cmd = argv[optind];
	vrf = argv[optind + 1];

	if (strchr(vrf, '/'))
		fprintf(stderr, "%s: namespace names containing slashes "
				"are not recommended.\n", vrf);
	if (vrf[0] == '.')
		fprintf(stderr, "%s: namespace names starting with dots "
				"are not recommended.\n", vrf);

	snprintf(vrfpath, sizeof(vrfpath), PATH_CFG "/%s", vrf);
	if (stat(vrfpath, &vrf_stat)) {
		fprintf(stderr, "%s: %s\n", vrfpath, strerror(errno));
		return 2;
	}
	if (!S_ISDIR(vrf_stat.st_mode)) {
		fprintf(stderr, "%s: not a directory\n", vrfpath);
		return 2;
	}

	if (verbose >= 1)
		fprintf(stderr, "%s: 0%o %d:%d\n", vrfpath, vrf_stat.st_mode,
				vrf_stat.st_uid, vrf_stat.st_gid);

	if (!strcmp(cmd, "start")) {
		return do_start(vrf);
	} else if (!strcmp(cmd, "getpid")) {
		return do_getpid(vrf);
	} else {
		fprintf(stderr, "unknown command '%s'\n", cmd);
		return 1;
	}
}

