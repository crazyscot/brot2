AC_INIT([brot2], [1.1+])
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

case $host_cpu in
  x86_64 )
	default_float=no
	default_double=no
	;;
  *)
	default_float=yes
	default_double=yes
	AC_WARN([Floating point limits are untested on this architecture; see PORTING])
	;;
esac

AC_ARG_ENABLE([float],
	[AS_HELP_STRING([--enable-float],
		[Enable use of the `float' type.])],
	[enable_float=${enableval}],
	[enable_float=${default_float}])

AC_ARG_ENABLE([double],
	[AS_HELP_STRING([--enable-double],
		[Enable use of the `double' type.])],
	[enable_double=${enableval}],
	[enable_double=${default_double}])

if test "x$enable_float" != xno; then
	AC_DEFINE([ENABLE_FLOAT], [1], [enable use of the `float' type])
fi
if test "x$enable_double" != xno; then
	AC_DEFINE([ENABLE_DOUBLE], [1], [enable use of the `double' type])
fi

AC_SUBST([BROT2_CFLAGS], [${pkg_cflags}])
AC_SUBST([BROT2_CXXFLAGS], [${pkg_cxxflags}])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

