dnl GTEST_SRC_CHECK
dnl Looks for the Google Test source, defaulting to on.
dnl Users can --disable-gtest or provide GTEST_SRC dir.
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

