#SUBDIRS=libjob libfractal libbrot2 cli gtkui gtest test

EXTRA_DIST=debian misc

AM_CPPFLAGS=@libbrot2_CFLAGS@ \
			-I$(top_srcdir) -I$(top_srcdir)/libfractal \
			-I$(top_srcdir)/libjob -I$(top_srcdir)/libbrot2
AM_CFLAGS=@BROT2_CFLAGS@
AM_CXXFLAGS=@BROT2_CXXFLAGS@

all_ldadd=$(top_builddir)/libjob/libjob.a \
		  $(top_builddir)/libfractal/libfractal.a \
		  $(top_builddir)/libbrot2/libbrot2.a \
		  @libbrot2_LIBS@ -lm

noinst_LIBRARIES=
bin_PROGRAMS=
check_LIBRARIES=
check_PROGRAMS=
TESTS=
AUTOGEN=
CLEANFILES=

################################################################

noinst_LIBRARIES+=libjob/libjob.a
VPATH+=$(top_srcdir)/libjob

libjob_libjob_a_SOURCES= \
	 IJob.h IJobEngine.h \
	 SimpleJobEngine.h SimpleJobEngine.cpp \
	 SimpleAsyncJobEngine.h SimpleAsyncJobEngine.cpp \
	 MultiThreadJobEngine.h MultiThreadJobEngine.cpp

################################################################

noinst_LIBRARIES+=libfractal/libfractal.a
VPATH+=$(top_srcdir)/libfractal

libfractal_libfractal_a_SOURCES= \
						Fractal.h Fractal.cpp Registry.h \
						Mandelbar.cpp Mandelbrots.cpp \
						Mandeldrop.cpp Misc.cpp

################################################################

noinst_LIBRARIES+=libbrot2/libbrot2.a
VPATH+=$(top_srcdir)/libbrot2

libbrot2_libbrot2_a_SOURCES= \
					Exception.h \
					noncopyable.hpp \
					logo.h $(top_builddir)/libbrot2/logo_auto.c \
					palette.cpp palette.h \
					Plot2.cpp Plot2.h \
					Render.h Render.cpp \
					Prefs.h Prefs.cpp \
					PrefsRegistry.h PrefsRegistry.cpp

AUTOGEN+=$(top_builddir)/libbrot2/logo_auto.c
CLEANFILES+=$(AUTOGEN)

LOGO_IN=$(top_srcdir)/misc/brot2.png

$(top_builddir)/libbrot2/logo_auto.c: $(LOGO_IN)
	@mkdir -p $(@D)
	$(AM_V_at) echo '#include "logo.h"' > $@.new
	$(AM_V_GEN) gdk-pixbuf-csource --raw --extern --name=brot2_logo $(LOGO_IN) >> $@.new
	$(AM_V_at) mv -f $@.new $@

################################################################

bin_PROGRAMS+=brot2cli
VPATH+=$(top_srcdir)/cli

brot2cli_SOURCES=climain.cpp climain.h \
				 reporter.cpp reporter.h

brot2cli_LDADD= $(all_ldadd)

################################################################

bin_PROGRAMS+=gtkui/brot2
#TODO figure out why this doesn't like vpaths.

gtkui_brot2_SOURCES= \
			gtkui/gtkmain.cpp gtkui/gtkmain.h gtkui/misc.h \
			gtkui/MainWindow.cpp gtkui/MainWindow.h \
			gtkui/Menus.cpp gtkui/Menus.h \
			gtkui/ParamsDialog.cpp gtkui/ParamsDialog.h \
			gtkui/PrefsDialog.cpp gtkui/PrefsDialog.h \
			gtkui/ControlsWindow.cpp gtkui/ControlsWindow.h \
			gtkui/Canvas.cpp gtkui/Canvas.h gtkui/HUD.cpp gtkui/HUD.h \
			gtkui/DragRectangle.cpp gtkui/DragRectangle.h \
			gtkui/ColourPanel.cpp gtkui/ColourPanel.h \
			gtkui/SaveAsPNG.cpp gtkui/SaveAsPNG.h

gtkui_brot2_CPPFLAGS=$(AM_CPPFLAGS) @brot2gui_CFLAGS@
gtkui_brot2_LDADD=$(all_ldadd) @brot2gui_LIBS@

################################################################

if GTEST
GTEST=@GTEST_SRC@/src
#VPATH=@GTEST_SRC@:@GTEST_SRC@/src

check_LIBRARIES+=gtest/libgtest.a

nodist_gtest_libgtest_a_SOURCES= $(GTEST)/gtest_main.cc $(GTEST)/gtest-all.cc

gtest_libgtest_a_CPPFLAGS= -I$(GTEST_INC) -I$(GTEST_SRC)
# Yes, include GTEST_SRC, because gtest-all includes some .cc files ...
endif

################################################################

if GTEST
check_PROGRAMS+=test/b2test
#TESTS+=test/b2test test/b2test.valgrind
endif

EXTRA_DIST+=test/b2test.valgrind test/b2test.vgsupp test/thrash

test_b2test_SOURCES=test/b2test.cpp \
					test/PrefsT.cpp test/PrefsT.h \
					test/JobsTest.cpp test/JobsTestBits.h

test_b2test_LDADD= gtest/libgtest.a $(all_ldadd)

test_b2test_CPPFLAGS=$(AM_CPPFLAGS) -I$(GTEST_INC)

if GTEST
check : $(check_PROGRAMS)
	$(top_builddir)/test/b2test
	G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --error-exitcode=99 --suppressions=$(top_srcdir)/test/b2test.vgsupp $(top_builddir)/test/b2test
	@echo
	@echo "All tests passed."
	@echo
else
check :
	@echo
	@echo Make check requires GTEST
	@false

endif
