#SUBDIRS=libfractal libbrot2 cli gtkui gtest test

EXTRA_DIST=debian misc

AM_CPPFLAGS=@libbrot2_CFLAGS@ \
			-I$(top_srcdir) -I$(top_srcdir)/libfractal \
			-I$(top_srcdir)/libbrot2
AM_CFLAGS=@BROT2_CFLAGS@
AM_CXXFLAGS=@BROT2_CXXFLAGS@

all_libs= \
		  $(top_builddir)/libfractal/libfractal.a \
		  $(top_builddir)/libbrot2/libbrot2.a

all_ldadd=$(all_libs) @libbrot2_LIBS@ -lm

noinst_LIBRARIES=
bin_PROGRAMS=
check_LIBRARIES=
check_PROGRAMS=
TESTS=
AUTOGEN=
CLEANFILES=

################################################################

noinst_LIBRARIES+=libfractal/libfractal.a

libfractal_libfractal_a_SOURCES= \
	libfractal/Fractal.h libfractal/Fractal.cpp libfractal/Registry.h \
	libfractal/Mandelbar.cpp libfractal/Mandelbrots.cpp \
	libfractal/Mandeldrop.cpp libfractal/Misc.cpp

################################################################

noinst_LIBRARIES+=libbrot2/libbrot2.a

libbrot2_libbrot2_a_SOURCES= \
	libbrot2/Exception.h \
	libbrot2/logo.h $(top_builddir)/libbrot2/logo_auto.c \
	libbrot2/palette.cpp libbrot2/palette.h \
	libbrot2/Plot3Plot.cpp libbrot2/Plot3Plot.h \
	libbrot2/Plot3Chunk.cpp libbrot2/Plot3Chunk.h \
	libbrot2/Plot3Pass.cpp libbrot2/Plot3Pass.h \
	libbrot2/ThreadPool.h libbrot2/ThreadPool.cpp \
	libbrot2/IPlot3DataSink.h \
	libbrot2/ChunkDivider.h libbrot2/ChunkDivider.cpp \
	libbrot2/Render2.h libbrot2/Render2.cpp \
	libbrot2/Prefs.h libbrot2/Prefs.cpp \
	libbrot2/PrefsRegistry.h libbrot2/PrefsRegistry.cpp

AUTOGEN+=$(top_builddir)/libbrot2/logo_auto.c
CLEANFILES+=$(AUTOGEN)

LOGO_IN=$(top_srcdir)/misc/brot2.png

$(top_builddir)/libbrot2/logo_auto.c: $(LOGO_IN)
	@mkdir -p $(@D)
	$(AM_V_at) echo '#include "logo.h"' > $@.new
	$(AM_V_GEN) gdk-pixbuf-csource --raw --extern --name=brot2_logo $(LOGO_IN) >> $@.new
	$(AM_V_at) mv -f $@.new $@

################################################################

bin_PROGRAMS+=cli/brot2cli

cli_brot2cli_SOURCES=cli/climain.cpp cli/climain.h \
				 cli/CLIDataSink.cpp cli/CLIDataSink.h

cli_brot2cli_LDADD= $(all_ldadd)

################################################################

bin_PROGRAMS+=gtkui/brot2

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
export builddir
TESTS+=test/b2test
if USE_VALGRIND
TESTS+=test/b2test.valgrind
endif

else
check :
	@echo
	@echo Make check requires GTEST
	@false
endif

EXTRA_DIST+=test/b2test.valgrind test/b2test.vgsupp test/thrash

test_b2test_SOURCES=test/b2test.cpp \
					test/PrefsT.cpp test/PrefsT.h \
					test/MockFractal.h test/MockFractal.cpp \
					test/MockPrefs.h test/MockPrefs.cpp \
					test/Plot3Test.cpp test/Render2Test.cpp

test_b2test_LDADD= gtest/libgtest.a $(all_ldadd)
test_b2test_DEPENDENCIES= gtest/libgtest.a $(all_libs)

test_b2test_CPPFLAGS=$(AM_CPPFLAGS) -I$(GTEST_INC)

