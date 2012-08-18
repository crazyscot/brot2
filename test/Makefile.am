if GTEST
check_PROGRAMS=b2test
TESTS=b2test b2test.valgrind
endif

EXTRA_DIST=b2test.valgrind b2test.vgsupp

b2test_SOURCES=b2test.cpp PrefsT.cpp PrefsT.h JobsTest.cpp JobsTestBits.h

b2test_LDADD= ../libbrot2/libbrot2.a ../libjob/libjob.a ../gtest/libgtest.a @libbrot2_LIBS@ -lm
b2test_DEPENDENCIES= ../libbrot2/libbrot2.a ../gtest/libgtest.a

AM_CPPFLAGS= -I$(top_srcdir) \
			 -I$(top_srcdir)/libbrot2 \
			 -I$(top_srcdir)/libjob \
			 @libbrot2_CFLAGS@ \
			 -I$(GTEST_INC)

AM_CFLAGS=@BROT2_CFLAGS@
AM_CXXFLAGS=@BROT2_CXXFLAGS@
