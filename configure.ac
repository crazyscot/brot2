AC_INIT([brot2], [2.0.2+])
AC_CONFIG_AUX_DIR([config.aux])
AM_INIT_AUTOMAKE([no-define silent-rules subdir-objects -Wno-portability])
AM_SILENT_RULES
AC_CONFIG_SRCDIR([libbrot2/Plot3Plot.cpp])
AM_CONFIG_HEADER([config.h])
AC_CANONICAL_HOST
AC_LANG([C++])
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AS
AC_PROG_RANLIB
AC_SET_MAKE
AM_MAINTAINER_MODE([enable])
AC_CHECK_LIB(X11, XInitThreads)
AC_CHECK_LIB(pthread, pthread_create)
AC_GNU_SOURCE
AC_C_BIGENDIAN

pkg_cflags=""
pkg_cxxflags="-std=c++11"
pkg_cppflags="-Ofast"
if test "x$GXX" = xyes; then
	pkg_cxxflags="${pkg_cxxflags} -Wall -Wextra -Werror -Wpointer-arith -Wwrite-strings -Woverloaded-virtual -Wnon-virtual-dtor -Wno-ctor-dtor-privacy -Wstrict-null-sentinel -Wnon-template-friend -Woverloaded-virtual -Wpmf-conversions -Wsign-promo -Wundef"
    # TODO: -Wdeprecated-copy-dtor, requires gcc9
    # libsigc++ can't be included with -Wctor-dtor-privacy at the present time
fi
if test "x$GCC" = xyes; then
	pkg_cflags="${pkg_cflags} -Wall -Wextra -Werror -Wpointer-arith -Wwrite-strings"
fi

LIBAV_CHECK
PKG_CHECK_MODULES([glibmm],[glibmm-2.4])
PKG_CHECK_MODULES([pango],[pangomm-1.4 pangocairo cairomm-1.0 cairo pango])
PKG_CHECK_MODULES([gtkmm],[gtkmm-2.4 gdkmm-2.4 sigc++-2.0])
PKG_CHECK_MODULES([gdkmm],[gdkmm-2.4 sigc++-2.0])
PKG_CHECK_MODULES([libpng],[libpng])
PKG_CHECK_MODULES([protobuf],[protobuf])


AC_CACHE_CHECK([for png++],
	[ac_cv_pngpp],
	[AC_COMPILE_IFELSE(
	 [AC_LANG_PROGRAM([#include <png++/png.hpp>], [return 0;])],
	 [ac_cv_pngpp=yes],
	 [AC_MSG_ERROR([<png++/png.hpp> not found or unusable.

	  Please download and install png++ <http://download.savannah.gnu.org/releases/pngpp/>.
	  See config.log for failure messages if required.])
	 ]
	 )]
	)

AC_CACHE_CHECK([if png++ is new enough],
	[ac_cv_pngpp_ver],
	[AC_COMPILE_IFELSE(
	 [AC_LANG_PROGRAM([#include <png++/png.hpp>], [std::string fn; png::image<png::rgb_pixel> png(fn); png.get_height(); return 0;])],
	 [ac_cv_pngpp_ver=yes],
	 [AC_MSG_ERROR([png++ is too old.

	  Please download and install png++ 0.2.5 or newer <http://download.savannah.gnu.org/releases/pngpp/>])
	 ]
	 )]
	)

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

AC_SUBST([BROT2_GLOBAL_CFLAGS], [${pkg_cflags}])
AC_SUBST([BROT2_GLOBAL_CXXFLAGS], [${pkg_cxxflags}])
AC_SUBST([BROT2_GLOBAL_CPPFLAGS], [${pkg_cppflags}])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

