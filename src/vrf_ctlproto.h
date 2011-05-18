/* vrf-tools
 *
 * Copyright (c) 2009, 2010, 2011 David Lamparter
 * please refer to the COPYING file for copying permission.
 */

#ifndef _VRF_CTLPROTO_H
#define _VRF_CTLPROTO_H

enum {
	VRF_CMD_NOOP = 0,
	VRF_CMD_SOCKET = 1,
	VRF_CMD_TAP = 2,
	VRF_CMD_EXEC = 3,
};

#endif /* _VRF_CTLPROTO_H */
