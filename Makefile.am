bin_PROGRAMS=brot2

PKGCONFIG_PKGS=glibmm-2.4 gtkmm-2.4 gdkmm-2.4 pango cairo pangocairo
brot2_SOURCES=$(AUTOSRC) \
	  main.cpp palette.cpp Fractal.cpp Mandelbrots.cpp Plot2.cpp \
			  		Mandelbar.cpp
brot2_LDFLAGS=`pkg-config $(PKGCONFIG_PKGS) --libs` \
			`libpng12-config --ldflags` \
			-lm

COMMON_CFLAGS = -O3

AM_CPPFLAGS=`pkg-config $(PKGCONFIG_PKGS) --cflags` \
			`libpng12-config --cflags`

AM_CFLAGS=$(COMMON_CFLAGS)
AM_CXXFLAGS=$(COMMON_CFLAGS) -std=c++0x
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
