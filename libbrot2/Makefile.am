noinst_LIBRARIES=libbrot2.a

libbrot2_a_SOURCES= \
					Exception.h \
					noncopyable.hpp \
					job/IJob.h job/IJobEngine.h \
					job/SimpleJobEngine.h job/SimpleJobEngine.cpp \
					job/SimpleAsyncJobEngine.h job/SimpleAsyncJobEngine.cpp \
					job/MultiThreadJobEngine.h job/MultiThreadJobEngine.cpp \
					logo.h logo_auto.c Registry.h \
					Fractal.cpp Fractal.h palette.cpp palette.h \
					Plot2.cpp Plot2.h \
					Mandelbar.cpp Mandelbrots.cpp \
					Mandeldrop.cpp \
					Misc.cpp \
					Render.h Render.cpp \
					Prefs.h Prefs.cpp PrefsRegistry.h PrefsRegistry.cpp

AM_CPPFLAGS=@libbrot2_CFLAGS@
AM_CFLAGS=@BROT2_CFLAGS@
AM_CXXFLAGS=@BROT2_CXXFLAGS@

AUTOGEN=logo_auto.c

LOGO_IN=../misc/brot2.png

logo_auto.c: $(LOGO_IN)
	$(AM_V_at) echo '#include "logo.h"' > $@.new
	$(AM_V_GEN) gdk-pixbuf-csource --raw --extern --name=brot2_logo $(LOGO_IN) >> $@.new
	$(AM_V_at) mv -f $@.new $@

CLEANFILES=$(AUTOGEN)

