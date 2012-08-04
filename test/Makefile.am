VPATH=@GTEST_SRC@:@GTEST_SRC@/src

if GTEST
all: check

TESTS=b2test

noinst_PROGRAMS=b2test
noinst_LIBRARIES=libgtest.a

b2test_SOURCES=b2test.cpp
b2test_LDADD= ../libbrot2/libbrot2.a @libbrot2_LIBS@ -lm libgtest.a
b2test_DEPENDENCIES= ../libbrot2/libbrot2.a libgtest.a

libgtest_a_SOURCES= gtest_main.cc gtest-all.cc

AM_CPPFLAGS= -I$(top_srcdir) -I$(top_srcdir)/libbrot2 @libbrot2_CFLAGS@ \
	-I$(GTEST_INC) -I$(GTEST_SRC)
# Yes, include GTEST_SRC, because gtest-all includes some .cc files ...

AM_CFLAGS=@global_CFLAGS@
AM_CXXFLAGS=@global_CXXFLAGS@

endif
