SUBDIRS=libbrot2

bin_PROGRAMS=brot2

brot2_SOURCES=$(AUTOGEN_SOURCES) main.cpp

brot2_LDADD= libbrot2/libbrot2.a @brot2_LIBS@ -lm
brot2_DEPENDENCIES= libbrot2/libbrot2.a

AM_CPPFLAGS= -Ilibbrot2 @brot2_CFLAGS@

AM_CFLAGS=@global_CFLAGS@
AM_CXXFLAGS=@global_CXXFLAGS@
EXTRA_DIST=debian

AUTOGEN_SOURCES=logo_auto.c uixml_auto.c

LOGO_IN=misc/brot2.png

logo_auto.c: $(LOGO_IN)
	echo '#include "logo.h"' > $@.new
	gdk-pixbuf-csource --raw --extern --name=brot2_logo $(LOGO_IN) >> $@.new
	mv -f $@.new $@

uixml_auto.c: ui.xml
	perl encodestring.pl uixml < $< > $@.new
	mv -f $@.new $@

CLEANFILES=$(AUTOGEN_SOURCES)

