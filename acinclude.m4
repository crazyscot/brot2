dnl GTEST_SRC_CHECK
dnl Looks for the Google Test source, defaulting to on.
dnl Users can --disable-gtest or provide GTEST_SRC dir.
dnl They may also need to provide GTEST_INC if the headers are in a
dnl different location that's not on the default search path.
AC_DEFUN([GTEST_SRC_CHECK],
[
AC_ARG_ENABLE([gtest],
	[AS_HELP_STRING([--enable-gtest],
		[Enable tests using the Google C++ Testing Framework.
		(Default is enabled.)])],
	[],
	[enable_gtest=default])

AC_ARG_VAR([GTEST_SRC], [Path to the Google Test sources.])
AC_ARG_VAR([GTEST_INC], [Path to the Google Test includes.])

AS_IF([test "x${GTEST_SRC}" = x], [GTEST_SRC="/usr/src/gtest"])
HAVE_GTEST="no"
AS_IF([test "x${enable_gtest}" != "xno"],
	[AC_MSG_CHECKING([for gtest.cc])
	 AS_IF([test -f "${GTEST_SRC}/src/gtest.cc"],
		 [HAVE_GTEST="yes"])
	 AS_IF([test "x${HAVE_GTEST}" = "xno"],
		 [AC_MSG_RESULT([no])
		  AS_IF([test "x${enable_gtest}" == "xdefault"],
			  [AC_MSG_WARN([dnl
Google Test sources not found, disabling autotests])],
			  [AC_MSG_ERROR([dnl
Unable to locate Google Test sources. Either specify the
location with GTEST_SRC, or disable unit tests with --disable-gtest.
		  ])])
		  ],
		 AC_MSG_RESULT([yes]))
	 ])
dnl Now the headers...
AS_IF([test "x${HAVE_GTEST}" = "xyes"],
	[AC_MSG_CHECKING([for gtest/gtest.h])
	 AS_IF([test "x${GTEST_INC}" == "x"],
		 [
			 dnl Attempt to autodetect
			 GTEST_INC="not found"
			 AS_IF([test -f "/usr/include/gtest/gtest.h"],
				 [GTEST_INC="/usr/include"])
			 AS_IF([test -f "${GTEST_SRC}/gtest/gtest.h"],
				 [GTEST_INC=${GTEST_SRC}])
			 AS_IF([test -f "${GTEST_SRC}/include/gtest/gtest.h"],
				 [GTEST_INC="${GTEST_SRC}/include"])
		 ])
	 dnl Now check it's sane
	 AS_IF([test -f "${GTEST_INC}/gtest/gtest.h"],
		  AC_MSG_RESULT([yes]),
		  [AS_IF([test "x${enable_gtest}" == "xdefault"],
			  [HAVE_GTEST=no
			   AC_MSG_WARN([dnl
Google Test headers not found, disabling autotests])],
			  AC_MSG_ERROR([Unable to detect Google Test headers. Specify the location with GTEST_INC.])
			  )]
		  )
	])

AM_CONDITIONAL([GTEST], [test "x${HAVE_GTEST}" = xyes])
AC_SUBST([GTEST_SRC])
AC_SUBST([GTEST_INC])
])

dnl VALGRIND_CHECK
dnl Looks for valgrind, defaulting to on.
dnl Users can --enable-valgrind or --disable-valgrind as they please.
dnl If enabled, valgrind must be on the PATH.
dnl If left at default, and valgrind is not found, it will be disabled.
dnl The result is the TEST_WITH_VALGRIND conditional.
AC_DEFUN([VALGRIND_CHECK],
[
AC_CHECK_PROG(HAVE_VALGRIND, valgrind, yes, no)
AC_ARG_ENABLE([valgrind],
	[  --enable-valgrind       Use valgrind when running unit tests. ],
	[case "${enableval}" in
		yes) use_valgrind=true ;;
		no)  use_valgrind=false;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-valgrind]) ;;
	esac], [use_valgrind=not_set])

if [[ "$use_valgrind" = "true" ]]; then
	if [[ "$HAVE_VALGRIND" = "no" ]]; then
		AC_MSG_ERROR([Valgrind not found in PATH.])
	fi
fi

if [[ "$use_valgrind" = "not_set" ]]; then
    # Default: autodetect
	if [[ "$HAVE_VALGRIND" = "yes" ]]; then
		use_valgrind=true
	else
		use_valgrind=false
	fi
fi

AM_CONDITIONAL([TEST_WITH_VALGRIND], [test "x${use_valgrind}" = xtrue])
])

dnl LIBAV_CHECK
dnl Looks for the ffmpeg/libav libraries.
dnl Users can --enable-libav or --disable-libav as they please.
dnl If left at default, and libav is not found, it will be disabled.
dnl The result is the TEST_WITH_LIBAV conditional.
AC_DEFUN([LIBAV_CHECK],
[
AC_ARG_WITH([libav],
	    AS_HELP_STRING([--with-libav[=DIR]], [Build with libav support]),
	    [with_libav=$withval],
	    [with_libav=yes])

AS_IF([test "x$with_libav" != "xno"], [
       AS_IF([test "x$with_libav" != "xyes"], [
	      PKG_CONFIG_PATH=${with_libav}/lib/pkgconfig:$PKG_CONFIG_PATH
	      export PKG_CONFIG_PATH
	])

       AC_SUBST(LIBAV_LIBS)
       AC_SUBST(LIBAV_CFLAGS)
       LIBAV_DEPS="libavutil libavformat libavcodec libswscale libswresample"
       if pkg-config $LIBAV_DEPS; then
               LIBAV_CFLAGS=`pkg-config --cflags $LIBAV_DEPS`
               LIBAV_LIBS=`pkg-config --libs $LIBAV_DEPS`
               HAVE_LIBAV="yes"
       else
              AC_MSG_ERROR([The ffmpeg packages 'libavutil-dev libavformat-dev libavcodec-dev libswscale-dev libswresample-dev' were requested, but not found. Please check your installation, install any necessary dependencies or use the '--without-libav' configuration option.])
       fi
])

AM_CONDITIONAL([LIBAV], [test "x${with_libav}" = xyes])
])
