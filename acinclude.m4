dnl AC_CPP_PRAGMA_ONCE
dnl - check for #pragma once
AC_DEFUN([AC_CPP_PRAGMA_ONCE], [{
	AC_MSG_CHECKING([[whether $CPP supports #pragma once]])
	AC_PREPROC_IFELSE(
		[AC_LANG_PROGRAM([[#pragma once]])],
		[
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE_PRAGMA_ONCE], [1], [Preprocessor support for #pragma once])
		],
		[AC_MSG_RESULT([no])])
	}])

dnl AC_C_FLAG([-flag])
dnl - check for CFLAG support in CC
AC_DEFUN([AC_C_FLAG], [{
	AC_LANG_PUSH(C)
	ac_c_flag_save="$CFLAGS"
	CFLAGS="$CFLAGS $1"
	AC_MSG_CHECKING([[whether $CC supports $1]])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([[]])],
		[
			AC_MSG_RESULT([yes])
			m4_if([$3], [], [], [
				CFLAGS="$ac_c_flag_save"
				$3
			])
		], [
			CFLAGS="$ac_c_flag_save"
			AC_MSG_RESULT([no])
			$2
		])
	AC_LANG_POP(C)
	}])

