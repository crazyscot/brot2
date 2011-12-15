bin_PROGRAMS=brot2cli

brot2cli_SOURCES=$(AUTOGEN_SOURCES) climain.cpp climain.h \
				 reporter.cpp reporter.h

brot2cli_LDADD= ../libbrot2/libbrot2.a @libbrot2_LIBS@ -lm
brot2cli_DEPENDENCIES= ../libbrot2/libbrot2.a

AM_CPPFLAGS= -I$(top_srcdir) -I$(top_srcdir)/libbrot2 @libbrot2_CFLAGS@

AM_CFLAGS=@global_CFLAGS@
AM_CXXFLAGS=@global_CXXFLAGS@

