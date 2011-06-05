bin_PROGRAMS=brot2

brot2_SOURCES=$(AUTOGEN_SOURCES) gtkmain.cpp gtkmain.h \
			  MainWindow.cpp MainWindow.h Menus.cpp Menus.h \
			  Canvas.cpp Canvas.h

brot2_LDADD= ../libbrot2/libbrot2.a @brot2_LIBS@ -lm
brot2_DEPENDENCIES= ../libbrot2/libbrot2.a

AM_CPPFLAGS= -I.. -I../libbrot2 @brot2_CFLAGS@

AM_CFLAGS=@global_CFLAGS@
AM_CXXFLAGS=@global_CXXFLAGS@

AUTOGEN_SOURCES=uixml_auto.c

uixml_auto.c: ui.xml
	$(AM_V_GEN) perl encodestring.pl uixml < $< > $@.new
	$(AM_V_at) mv -f $@.new $@

CLEANFILES=$(AUTOGEN_SOURCES)

