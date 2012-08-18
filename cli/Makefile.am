bin_PROGRAMS=brot2cli

brot2cli_SOURCES=$(AUTOGEN_SOURCES) climain.cpp climain.h \
				 reporter.cpp reporter.h

brot2cli_LDADD= $(top_srcdir)/libbrot2/libbrot2.a \
				$(top_srcdir)/libfractal/libfractal.a \
				@libbrot2_LIBS@ -lm
brot2cli_DEPENDENCIES= $(top_srcdir)/libbrot2/libbrot2.a \
					   $(top_srcdir)/libfractal/libfractal.a

AM_CPPFLAGS= -I$(top_srcdir) -I$(top_srcdir)/libbrot2 \
			 -I$(top_srcdir)/libfractal @libbrot2_CFLAGS@

AM_CFLAGS=@BROT2_CFLAGS@
AM_CXXFLAGS=@BROT2_CXXFLAGS@

