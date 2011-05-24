SUBDIRS=libbrot2

bin_PROGRAMS=brot2

brot2_SOURCES=$(AUTOSRC) main.cpp

brot2_LDADD= libbrot2/libbrot2.a @brot2_LIBS@ -lm

AM_CPPFLAGS= -Ilibbrot2 @brot2_CFLAGS@

AM_CFLAGS=@global_CFLAGS@
AM_CXXFLAGS=@global_CXXFLAGS@
EXTRA_DIST=debian

AUTOSRC=logo_auto.c uixml_auto.c

LOGO_SRC=misc/brot2.png

logo_auto.c: $(LOGO_SRC)
	echo '#include "logo.h"' > $@.new
	gdk-pixbuf-csource --raw --extern --name=brot2_logo $(LOGO_SRC) >> $@.new
	mv -f $@.new $@

uixml_auto.c: ui.xml
	perl encodestring.pl uixml < $< > $@.new
	mv -f $@.new $@

CLEANFILES=$(AUTOSRC)

