if GTEST
TESTS=b2test

noinst_PROGRAMS=b2test

b2test_SOURCES=b2test.cpp TestPrefs.cpp

b2test_LDADD= ../libbrot2/libbrot2.a ../gtest/libgtest.a @libbrot2_LIBS@ -lm
b2test_DEPENDENCIES= ../libbrot2/libbrot2.a ../gtest/libgtest.a

AM_CPPFLAGS= -I$(top_srcdir) -I$(top_srcdir)/libbrot2 @libbrot2_CFLAGS@ \
	-I$(GTEST_INC)

AM_CFLAGS=@global_CFLAGS@
AM_CXXFLAGS=@global_CXXFLAGS@

endif
