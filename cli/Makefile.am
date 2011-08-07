bin_PROGRAMS=brot2cli

brot2cli_SOURCES=$(AUTOGEN_SOURCES) climain.cpp reporter.cpp

brot2cli_LDADD= ../libbrot2/libbrot2.a @glibmm_LIBS@ -lm
brot2cli_DEPENDENCIES= ../libbrot2/libbrot2.a

AM_CPPFLAGS= -I.. -I../libbrot2 @glibmm_CFLAGS@

AM_CFLAGS=@global_CFLAGS@
AM_CXXFLAGS=@global_CXXFLAGS@

