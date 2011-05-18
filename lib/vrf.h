/* vrf-tools
 *
 * Copyright (c) 2009, 2010, 2011 David Lamparter
 * please refer to the COPYING file for copying permission.
 */

#ifndef _VRF_H
#define _VRF_H

extern int vrf_socket(const char *vrf, int domain, int type, int protocol);
extern int vrf_tap(const char *vrf);

#endif /* _VRF_H */
