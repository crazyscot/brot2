
SOURCES = brot2.c
OBJS    = ${SOURCES:.c=.o}
CFLAGS  = `pkg-config gtk+-2.0 --cflags` -g -O0
LDADD   = `pkg-config gtk+-2.0 --libs`
CC      = gcc
PACKAGE = brot2

all: brot2

brot2 : ${OBJS}
	  ${CC} -o $@ ${OBJS} ${LDADD}

.c.o:
	  ${CC} ${CFLAGS} -c $<

