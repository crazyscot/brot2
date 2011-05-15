
CSRC   := 
CXXSRC := main.cpp palette.cpp Fractal.cpp Mandelbrots.cpp Plot2.cpp \
		Mandelbar.cpp
COBJ   := $(CSRC:.c=.o)
CXXOBJ := $(CXXSRC:.cpp=.o)
OBJS   := $(COBJ) $(CXXOBJ)
DEPS   := $(CSRC:.c=.d) $(CXXSRC:.cpp=.d)

PKGCONFIG_PKGS := gtk+-2.0 gtkmm-2.4 glib-2.0 glibmm-2.4

COMMON_CFLAGS := `pkg-config $(PKGCONFIG_PKGS) --cflags` \
			`libpng12-config --cflags` \
			-g -O3 -Wall -Werror -std=c++0x
CFLAGS := $(COMMON_CFLAGS)
CXXFLAGS := $(COMMON_CFLAGS)
LDADD  := `pkg-config $(PKGCONFIG_PKGS) --libs` \
			`libpng12-config --ldflags`	\
			-lm
CC     := gcc
CXX    := g++

BINDIR	 := $(DESTDIR)/usr/bin
TARGETS  := brot2
ICONSDIR := $(DESTDIR)/usr/share/pixmaps
ICONS    := brot2.xpm
INSTALL	 := install

all: $(TARGETS)

brot2 : $(OBJS)
	  $(CXX) -o $@ $(OBJS) $(LDADD)

install: all
	$(INSTALL) -d $(BINDIR) $(ICONSDIR)
	$(INSTALL) brot2 $(BINDIR)
	$(INSTALL) -m644 $(ICONS) $(ICONSDIR)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGETS)

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
