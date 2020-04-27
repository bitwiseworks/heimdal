dnl $Id$
dnl rk_OS2
AC_DEFUN([rk_OS2],[AH_BOTTOM([
#ifdef __OS2__
#define NO_POSIX_LINKS
#define HAVE_CONIO_H
#define NO_UNIX_SOCKETS 1
#endif
])])
