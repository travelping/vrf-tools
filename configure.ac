AC_PREREQ(2.57)
AC_INIT(vrf-tools, 0.1.99)
AC_CONFIG_AUX_DIR(ac)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([acconf.h])

clients="yes"

AC_CANONICAL_HOST
case "$host_os" in
	linux*)
		;;
	*)
		dnl make it noticeable...
		AC_MSG_WARN([you are trying to build vrf-tools for a non-Linux system, this will not work])
		AC_MSG_WARN([you are trying to build vrf-tools for a non-Linux system, this will not work])
		AC_MSG_WARN([you are trying to build vrf-tools for a non-Linux system, this will not work])
		;;
esac

AC_PROG_CC
AC_C_CONST
AC_C_INLINE

AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_CPP_PRAGMA_ONCE
AC_C_FLAG([-Wall])
AC_C_FLAG([-Wextra],[AC_C_FLAG([-W])])
AC_C_FLAG([-Wno-unused-parameter])
AC_C_FLAG([-Winvalid-pch])
AC_C_FLAG([-Wdeclaration-after-statement])
AC_C_FLAG([-Wshadow])
AC_C_FLAG([-pedantic])
AC_C_FLAG([-std=c99],[AC_C_FLAG([-c99])])
AC_ARG_ENABLE([werror],
	AC_HELP_STRING([--enable-werror], [compile with -Werror (for developers)]),
	[	if test "$enableval" == "yes"
		then	AC_C_FLAG([-Werror],,[werror="-Werror"])
		fi])
AC_SUBST([werror])

AC_PATH_XTRA

sched_h=""
AC_CHECK_HEADERS([sched.h],[
	sched_h="#include <sched.h>"
])
linux_unistd_h=""
AC_CHECK_HEADERS([linux/unistd.h],[
	linux_unistd_h="#include <linux/unistd.h>"
])

AC_CHECK_FUNCS([clone unshare],, [
	AC_MSG_WARN([no clone/unshare, this will not work])
])
AC_CHECK_FUNCS([setns], [], [])

AC_CHECK_DECLS([CLONE_NEWNET],,, [[
#define _GNU_SOURCE
#include <unistd.h>
${sched_h}
${linux_unistd_h}
]])
AC_CHECK_DECLS([CLONE_NEWUTS],,, [[
#define _GNU_SOURCE
#include <unistd.h>
${sched_h}
${linux_unistd_h}
]])

CONFDATE=`date '+%Y%m%d'`
AC_SUBST(CONFDATE)

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	lib/Makefile
	doc/Makefile
	rpm/Makefile
	rpm/vrf-tools.spec
])

AC_OUTPUT
