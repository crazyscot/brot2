AC_INIT([brot2], [1.0])
AC_CONFIG_AUX_DIR([config.aux])
AM_INIT_AUTOMAKE([no-define silent-rules])
AC_CONFIG_SRCDIR([libbrot2/Plot2.cpp])
AM_CONFIG_HEADER([config.h])
AC_CANONICAL_HOST
AC_LANG([C++])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AM_PROG_AS
AC_PROG_RANLIB
AC_SET_MAKE
AM_MAINTAINER_MODE([enable])

pkg_cflags=""
pkg_cxxflags="-std=c++0x"
AC_DEFINE([_GNU_SOURCE], [1], [use GNU extensions])
if test "x$GXX" = xyes; then
	pkg_cxxflags="${pkg_cxxflags} -Wall -Wextra -Werror -Wpointer-arith -Wwrite-strings"
fi
if test "x$GCC" = xyes; then
	pkg_cflags="${pkg_cflags} -Wall -Wextra -Werror -Wpointer-arith -Wwrite-strings"
fi

PKG_CHECK_MODULES([libbrot2],[glibmm-2.4 libpng12])
PKG_CHECK_MODULES([brot2gui],[glibmm-2.4 gtkmm-2.4 gdkmm-2.4 pangocairo cairo pango libpng12])

GTEST_SRC_CHECK

AC_ARG_ENABLE([debug],
		  [AS_HELP_STRING([--enable-debug],
						  [Sets debug compile flags. (Default is disabled, for speed.)])],
		  [pkg_cflags="${pkg_cflags} -O0 -g"
		   pkg_cxxflags="${pkg_cxxflags} -O0 -g"],
		  [pkg_cflags="${pkg_cflags} -O3"
		   pkg_cxxflags="${pkg_cxxflags} -O3"]
		  )


AC_SUBST([BROT2_CFLAGS], [${pkg_cflags}])
AC_SUBST([BROT2_CXXFLAGS], [${pkg_cxxflags}])

AC_CONFIG_FILES([Makefile libbrot2/Makefile cli/Makefile gtkui/Makefile gtest/Makefile test/Makefile])
AC_OUTPUT

