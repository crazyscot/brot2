EXTRA_DIST=debian misc

AM_CPPFLAGS=@BROT2_GLOBAL_CPPFLAGS@ \
			-I$(top_srcdir) -I$(top_srcdir)/libfractal \
			-I$(top_srcdir)/libbrot2
AM_CFLAGS=@BROT2_GLOBAL_CFLAGS@
AM_CXXFLAGS=@BROT2_GLOBAL_CXXFLAGS@

all_libs= \
		  libfractal.a \
		  libbrot2.a

all_ldadd=$(all_libs) @libbrot2_a_LIBS@ -lm

noinst_LIBRARIES=
bin_PROGRAMS=
check_LIBRARIES=
check_PROGRAMS=
AUTOGEN=
CLEANFILES=

################################################################

noinst_LIBRARIES+=libfractal.a

libfractal_a_SOURCES= \
	libfractal/Fractal.h libfractal/Fractal.cpp libfractal/Registry.h \
	libfractal/FractalMaths.h libfractal/Fractal-internals.h \
	libfractal/Mandelbrots.cpp libfractal/Mandelbar.cpp \
	libfractal/Mandeldrop.cpp libfractal/Misc.cpp

################################################################

noinst_LIBRARIES+=libbrot2.a

libbrot2_a_SOURCES= \
	libbrot2/Exception.h \
	libbrot2/misc.h libbrot2/misc.cpp \
	libbrot2/logo.h logo_auto.c \
	libbrot2/palette.cpp libbrot2/palette.h \
	libbrot2/Plot3Plot.cpp libbrot2/Plot3Plot.h \
	libbrot2/Plot3Chunk.cpp libbrot2/Plot3Chunk.h \
	libbrot2/Plot3Pass.cpp libbrot2/Plot3Pass.h \
	libbrot2/ThreadPool.h libbrot2/ThreadPool.cpp \
	libbrot2/IPlot3DataSink.h \
	libbrot2/IMovieProgress.h libbrot2/MovieNullProgress.cpp \
	libbrot2/ChunkDivider.h libbrot2/ChunkDivider.cpp \
	libbrot2/Render2.h libbrot2/Render2.cpp \
	libbrot2/Prefs.h libbrot2/Prefs.cpp \
	libbrot2/BaseHUD.h libbrot2/BaseHUD.cpp \
	libbrot2/MovieMode.h libbrot2/MovieMode.cpp \
	libbrot2/MovieMotion.h libbrot2/MovieMotion.cpp \
	libbrot2/MovieRender.cpp libbrot2/MovieRender.h \
	libbrot2/PrefsRegistry.h libbrot2/PrefsRegistry.cpp

AUTOGEN+=logo_auto.c
CLEANFILES+=$(AUTOGEN)

LOGO_IN=$(top_srcdir)/misc/brot2.png

logo_auto.c: $(LOGO_IN)
	@mkdir -p $(@D)
	$(AM_V_at) echo '#include "logo.h"' > $@.new
	$(AM_V_GEN) gdk-pixbuf-csource --raw --extern --name=brot2_logo $(LOGO_IN) >> $@.new
	$(AM_V_at) mv -f $@.new $@

################################################################

bin_PROGRAMS+=brot2cli

brot2cli_SOURCES=cli/climain.cpp cli/climain.h \
				 cli/CLIDataSink.cpp cli/CLIDataSink.h

brot2cli_LDADD= $(all_ldadd)

################################################################

bin_PROGRAMS+=brot2

brot2_SOURCES= \
	gtkui/gtkmain.cpp gtkui/gtkmain.h gtkui/gtkutil.h gtkui/gtkutil.cpp \
	gtkui/MainWindow.cpp gtkui/MainWindow.h \
	gtkui/Menus.cpp gtkui/Menus.h \
	gtkui/ParamsDialog.cpp gtkui/ParamsDialog.h \
	gtkui/PrefsDialog.cpp gtkui/PrefsDialog.h \
	gtkui/ControlsWindow.cpp gtkui/ControlsWindow.h \
	gtkui/MovieWindow.cpp gtkui/MovieWindow.h \
	gtkui/MovieRender-PNG.cpp \
	gtkui/MovieRender-libav.cpp \
	gtkui/MovieProgress.cpp gtkui/MovieProgress.h \
	gtkui/Canvas.cpp gtkui/Canvas.h gtkui/HUD.cpp gtkui/HUD.h \
	gtkui/DragRectangle.cpp gtkui/DragRectangle.h \
	gtkui/ColourPanel.cpp gtkui/ColourPanel.h \
	gtkui/SaveAsPNG.cpp gtkui/SaveAsPNG.h

brot2_CPPFLAGS=$(AM_CPPFLAGS) @brot2gui_CFLAGS@ @libav_CFLAGS@
brot2_LDADD=$(all_ldadd) @brot2gui_LIBS@ @libav_LIBS@

################################################################

if GTEST
GTEST=@GTEST_SRC@/src
#VPATH=@GTEST_SRC@:@GTEST_SRC@/src

check_LIBRARIES+=libgtest.a

nodist_libgtest_a_SOURCES= gtest/src/gtest_main.cc gtest/src/gtest-all.cc

gtest/src/%: $(GTEST_SRC)/src/%
	mkdir -p gtest/src
	rm -f gtest/src/*
	cp -a $(GTEST_SRC)/src/* gtest/src/

libgtest_a_CPPFLAGS= -I$(GTEST_INC) -I$(GTEST_SRC)
libgtest_a_CXXFLAGS= $(AM_CPPFLAGS) -Wno-undef
# Yes, include GTEST_SRC, because gtest-all includes some .cc files ...

CLEANFILES+=gtest/src/*.cc gtest/src/*.h
endif

################################################################

if GTEST
check_PROGRAMS+=b2test
export builddir
if TEST_WITH_VALGRIND
TEST_VG=test/b2test.valgrind
endif TEST_WITH_VALGRIND
TESTS=b2test $(TEST_VG)

else !GTEST
check :
	@echo
	@echo Make check requires GTEST
	@false
endif !GTEST

EXTRA_DIST+=test/b2test.valgrind test/b2test.vgsupp test/thrash

b2test_SOURCES=test/b2test.cpp \
					test/PrefsT.cpp test/PrefsT.h \
					test/MockFractal.h test/MockFractal.cpp \
					test/MockPalette.h \
					test/MockPrefs.h test/MockPrefs.cpp \
					test/Plot3Test.cpp test/Render2Test.cpp \
					test/FractalKAT.cpp test/MovieTest.cpp

b2test_LDADD= libgtest.a $(all_ldadd)
b2test_DEPENDENCIES= libgtest.a $(all_libs)

b2test_CPPFLAGS=$(AM_CPPFLAGS) -I$(GTEST_INC)

################################################################

benchmark_PROGRAMS=plot_benchmark palette_benchmark
benchmarkdir=benchmark
benchmark_SOURCES=benchmark/Benchmarkable.cpp benchmark/Benchmarkable.h

check_PROGRAMS+=$(benchmark_PROGRAMS)
plot_benchmark_SOURCES=$(benchmark_SOURCES) benchmark/plot_benchmark.cpp

plot_benchmark_LDADD= libfractal.a
plot_benchmark_DEPENDENCIES= libfractal.a
plot_benchmark_CPPFLAGS=$(AM_CPPFLAGS)

palette_benchmark_SOURCES=$(benchmark_SOURCES) benchmark/palette_benchmark.cpp \
				 cli/CLIDataSink.cpp cli/CLIDataSink.h

palette_benchmark_LDADD= $(all_ldadd)
palette_benchmark_DEPENDENCIES= $(all_ldadd)
palette_benchmark_CPPFLAGS=$(AM_CPPFLAGS) @brot2gui_CFLAGS@

benchmark: $(benchmark_PROGRAMS)
	for p in $(benchmark_PROGRAMS); do ./$$p; done

################################################################
