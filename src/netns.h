#ifndef _NETNS_H
#define _NETNS_H

extern void safe_write(int fd, void *buf, size_t len);
extern int safe_read(int fd, void *buf, size_t len);

extern int ctl_open(const char *ns);
extern void ctl_run(int ctlsock);

#endif /* _NETNS_H */
