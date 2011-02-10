#ifndef _VRF_APP_H
#define _VRF_APP_H

extern void safe_write(int fd, void *buf, size_t len);
extern int safe_read(int fd, void *buf, size_t len);

extern int ctl_open(const char *vrf);
extern void ctl_run(int ctlsock);

#endif /* _VRF_APP_H */
