
SOURCES:= brot2.c mandelbrot.c
OBJS   := ${SOURCES:.c=.o}
DEPS   := $(SOURCES:.c=.d)
CFLAGS := `pkg-config gtk+-2.0 --cflags` -g -O0
LDADD  := `pkg-config gtk+-2.0 --libs`
CC     := gcc
PACKAGE:= brot2

all: brot2

brot2 : ${OBJS}
	  ${CC} -o $@ ${OBJS} ${LDADD}

 %.d: %.c
	 @set -e; rm -f $@; \
		 $(CC) -MM $(CFLAGS) $< > $@.$$$$; \
		 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		 rm -f $@.$$$$

.c.o:
	  ${CC} ${CFLAGS} -c $<

clean:
	rm -f $(OBJS) $(DEPS)

include $(DEPS)

