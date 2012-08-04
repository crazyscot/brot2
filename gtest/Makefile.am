VPATH=@GTEST_SRC@:@GTEST_SRC@/src

if GTEST
noinst_LIBRARIES=libgtest.a

libgtest_a_SOURCES= gtest_main.cc gtest-all.cc

AM_CPPFLAGS= -I$(GTEST_INC) -I$(GTEST_SRC)
# Yes, include GTEST_SRC, because gtest-all includes some .cc files ...

endif
