SUBDIRS=libbrot2

bin_PROGRAMS=brot2

PKGCONFIG_PKGS=glibmm-2.4 gtkmm-2.4 gdkmm-2.4 pango cairo pangocairo
brot2_SOURCES=$(AUTOSRC) main.cpp

brot2_LDFLAGS=`pkg-config $(PKGCONFIG_PKGS) --libs` \
			`libpng12-config --ldflags` \
			-lm
brot2_LDADD= libbrot2/libbrot2.a

AM_CPPFLAGS= -Ilibbrot2 \
			 `pkg-config $(PKGCONFIG_PKGS) --cflags` \
			`libpng12-config --cflags`

AM_CFLAGS=@global_CFLAGS@
AM_CXXFLAGS=@global_CXXFLAGS@
EXTRA_DIST=
AM_LDFLAGS= --g-fatal-warnings

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

