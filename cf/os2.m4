dnl $Id$
dnl rk_OS2
AC_DEFUN([rk_OS2],[AH_BOTTOM([
#ifdef __OS2__
#define NO_POSIX_LINKS
#endif
])])
