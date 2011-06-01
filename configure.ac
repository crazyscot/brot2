AC_INIT([brot2], [0.1.wip])
AC_CONFIG_AUX_DIR([config.aux])
AM_INIT_AUTOMAKE([no-define silent-rules])
AC_CONFIG_SRCDIR([libbrot2/Plot2.cpp])
AM_CONFIG_HEADER([config.h])
AC_CANONICAL_HOST
AC_LANG([C++])
: ${CFLAGS="-g"}
: ${CXXFLAGS="-g"}
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AM_PROG_AS
AC_PROG_RANLIB
AC_SET_MAKE
AC_DEFINE([_GNU_SOURCE], [1], [use GNU extensions])
if test "x$GXX" = xyes; then
	CXX="$CXX -Wall -W -Werror -Wpointer-arith -Wwrite-strings"
fi
if test "x$GCC" = xyes; then
	CC="$CC -Wall -W -Werror -Wpointer-arith -Wwrite-strings"
fi

AC_SUBST([global_CFLAGS], ["-O3"])
AC_SUBST([global_CXXFLAGS], ["-O3 -std=c++0x"])

PKG_CHECK_MODULES([glibmm],[glibmm-2.4])
PKG_CHECK_MODULES([brot2],[glibmm-2.4 gtk+-2.0 gdk-2.0 pangocairo cairo pango libpng12])

AC_CONFIG_FILES([Makefile libbrot2/Makefile])
AC_OUTPUT

