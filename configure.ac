AC_INIT([brot2], [0.1.XXX], [ross@impropriety.org.uk])
AC_CONFIG_AUX_DIR([config.aux])
AM_INIT_AUTOMAKE([brot2], [0.1.XXX])
AC_CONFIG_SRCDIR([Plot2.cpp])
AM_CONFIG_HEADER([config.h])
AC_CANONICAL_HOST
AC_LANG([C++])
: ${CFLAGS="-g"}
: ${CXXFLAGS="-g"}
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AS
AC_PROG_RANLIB
AC_SET_MAKE
AC_DEFINE([_GNU_SOURCE], [1], [use GNU extensions])
if test "x$GXX" = xyes; then
	CXX="$CXX -Wall -Werror -Wpointer-arith -Wwrite-strings"
fi
if test "x$GCC" = xyes; then
	CC="$CC -Wall -Werror -Wpointer-arith -Wwrite-strings"
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

