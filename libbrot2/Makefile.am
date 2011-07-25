noinst_LIBRARIES=libbrot2.a

libbrot2_a_SOURCES= libbrot2.h \
					logo.h logo_auto.c Registry.h \
					Fractal.cpp Fractal.h palette.cpp palette.h \
					Plot2.cpp Plot2.h \
					Mandelbar.cpp Mandelbrots.cpp

AM_CFLAGS=@global_CFLAGS@
AM_CXXFLAGS=@global_CXXFLAGS@ @glibmm_CFLAGS@

AUTOGEN=logo_auto.c

LOGO_IN=../misc/brot2.png

logo_auto.c: $(LOGO_IN)
	$(AM_V_at) echo '#include "logo.h"' > $@.new
	$(AM_V_GEN) gdk-pixbuf-csource --raw --extern --name=brot2_logo $(LOGO_IN) >> $@.new
	$(AM_V_at) mv -f $@.new $@

CLEANFILES=$(AUTOGEN)

