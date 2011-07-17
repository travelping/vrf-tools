DEFS = -DPATH_CFG=\"$(sysconfdir)\" -DPATH_STATE=\"$(localstatedir)\"
AM_CFLAGS = -I..

include_HEADERS = vrf.h
lib_LTLIBRARIES = libvrf.la
libvrf_la_SOURCES = vrf_kernel_setns.c
EXTRA_DIST = vrf_dummy.c vrf_fdpass.c

