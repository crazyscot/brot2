AC_INIT([brot2], [1.0])
AC_CONFIG_AUX_DIR([config.aux])
AM_INIT_AUTOMAKE([no-define silent-rules])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([libbrot2/Plot3Plot.cpp])
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
	pkg_cxxflags="${pkg_cxxflags} -Wall -Wextra -Werror -Wpointer-arith -Wwrite-strings -Woverloaded-virtual -Wnon-virtual-dtor -Wctor-dtor-privacy -Wstrict-null-sentinel -Wnon-template-friend -Woverloaded-virtual -Wpmf-conversions -Wsign-promo"
fi
if test "x$GCC" = xyes; then
	pkg_cflags="${pkg_cflags} -Wall -Wextra -Werror -Wpointer-arith -Wwrite-strings"
fi

PKG_CHECK_MODULES([libbrot2],[glibmm-2.4 libpng12 sigc++-2.0])
PKG_CHECK_MODULES([brot2gui],[glibmm-2.4 gtkmm-2.4 gdkmm-2.4 pangocairo cairo pango libpng12 sigc++-2.0])

GTEST_SRC_CHECK
VALGRIND_CHECK

AC_SUBST([BROT2_CFLAGS], [${pkg_cflags}])
AC_SUBST([BROT2_CXXFLAGS], [${pkg_cxxflags}])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

