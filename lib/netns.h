#ifndef _NETNS_H
#define _NETNS_H

extern int netns_socket(const char *netns, int domain, int type, int protocol);
extern int netns_tap(const char *netns);

#endif /* _NETNS_H */
