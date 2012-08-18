bin_PROGRAMS=brot2

brot2_SOURCES=$(AUTOGEN_SOURCES) gtkmain.cpp gtkmain.h misc.h \
			  MainWindow.cpp MainWindow.h Menus.cpp Menus.h \
			  ParamsDialog.cpp ParamsDialog.h \
			  PrefsDialog.cpp PrefsDialog.h \
			  ControlsWindow.cpp ControlsWindow.h \
			  Canvas.cpp Canvas.h HUD.cpp HUD.h \
			  DragRectangle.cpp DragRectangle.h \
			  ColourPanel.cpp ColourPanel.h \
			  SaveAsPNG.cpp SaveAsPNG.h

brot2_LDADD= $(top_builddir)/libbrot2/libbrot2.a \
			 $(top_builddir)/libfractal/libfractal.a \
			 @brot2gui_LIBS@ -lm

brot2_DEPENDENCIES= $(top_builddir)/libbrot2/libbrot2.a \
					$(top_builddir)/libfractal/libfractal.a

AM_CPPFLAGS= -I$(top_srcdir) -I$(top_srcdir)/libbrot2 \
			 -I$(top_srcdir)/libfractal @brot2gui_CFLAGS@

AM_CFLAGS=@BROT2_CFLAGS@
AM_CXXFLAGS=@BROT2_CXXFLAGS@

