AC_INIT([brot2], [0.4.999.wip])
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
	CXX="$CXX -Wall -Wextra -Werror -Wpointer-arith -Wwrite-strings -Woverloaded-virtual"
fi
if test "x$GCC" = xyes; then
	CC="$CC -Wall -Wextra -Werror -Wpointer-arith -Wwrite-strings"
fi

AC_SUBST([global_CFLAGS], ["-O3"])
AC_SUBST([global_CXXFLAGS], ["-O3 -std=c++0x"])

PKG_CHECK_MODULES([libbrot2],[glibmm-2.4 libpng12])
PKG_CHECK_MODULES([brot2gui],[glibmm-2.4 gtkmm-2.4 gdkmm-2.4 pangocairo cairo pango libpng12])

AC_CONFIG_FILES([Makefile libbrot2/Makefile cli/Makefile gtkui/Makefile])
AC_OUTPUT

