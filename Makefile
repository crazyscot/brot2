
CSRC   := logo_auto.c uixml_auto.c
CXXSRC := main.cpp palette.cpp Fractal.cpp Mandelbrots.cpp Plot2.cpp \
		Mandelbar.cpp
COBJ   := $(CSRC:.c=.o)
CXXOBJ := $(CXXSRC:.cpp=.o)
OBJS   := $(COBJ) $(CXXOBJ)
DEPS   := $(CSRC:.c=.d) $(CXXSRC:.cpp=.d)

PKGCONFIG_PKGS := glibmm-2.4 gtk+-2.0 pango cairo pangocairo

COMMON_CFLAGS := `pkg-config $(PKGCONFIG_PKGS) --cflags` \
			`libpng12-config --cflags` \
			-g -O3 -Wall -Werror
CFLAGS := $(COMMON_CFLAGS)
CXXFLAGS := $(COMMON_CFLAGS) -std=c++0x
LDADD  := `pkg-config $(PKGCONFIG_PKGS) --libs` \
			`libpng12-config --ldflags`	\
			-lm
#LDFLAGS := --g-fatal-warnings

CC     := gcc
CXX    := g++

INSTALL	 := install

BINDIR	 := $(DESTDIR)/usr/bin
TARGETS  := brot2
ICONSDIR := $(DESTDIR)/usr/share/pixmaps
ICONS    := misc/brot2.xpm misc/brot2.png
DESKDIR  := $(DESTDIR)/usr/share/applications
DESKTOP  := misc/brot2.desktop

all: $(TARGETS)

brot2 : $(OBJS)
	  $(CXX) -o $@ $(OBJS) $(LDADD) $(LDFLAGS)

LOGO_SRC:=misc/brot2.png

logo_auto.c: $(LOGO_SRC)
	echo '#include "logo.h"' > $@.new
	gdk-pixbuf-csource --raw --extern --name=brot2_logo $(LOGO_SRC) >> $@.new
	mv -f $@.new $@

uixml_auto.c: ui.xml
	perl encodestring.pl uixml < $< > $@.new
	mv -f $@.new $@


AUTOSRC:= logo_auto.c uixml_auto.c

install: all
	$(INSTALL) -d $(BINDIR) $(ICONSDIR) $(DESKDIR)
	$(INSTALL) brot2 $(BINDIR)
	$(INSTALL) -m644 $(ICONS) $(ICONSDIR)
	$(INSTALL) -m644 $(DESKTOP) $(DESKDIR)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGETS) $(AUTOSRC)

%.d: %.c
	 @set -e; rm -f $@; \
		 $(CC) -MM $(CFLAGS) $< > $@.$$$$; \
		 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		 rm -f $@.$$$$

%.d: %.cpp
	 @set -e; rm -f $@; \
		 $(CXX) -MM $(CXXFLAGS) $< > $@.$$$$; \
		 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		 rm -f $@.$$$$

.c.o:
	  $(CC) $(CFLAGS) -c $<

.cpp.o:
	  $(CXX) $(CXXFLAGS) -c $<

include $(DEPS)
