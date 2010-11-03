
CSRC   := brot2.c mandelbrot.c
CXXSRC := palette.cpp
COBJ   := $(CSRC:.c=.o)
CXXOBJ := $(CXXSRC:.cpp=.o)
OBJS   := $(COBJ) $(CXXOBJ)
DEPS   := $(CSRC:.c=.d) $(CXXSRC:.cpp=.d)

CFLAGS := `pkg-config gtk+-2.0 --cflags` -g -O0
CXXFLAGS := `pkg-config gtk+-2.0 --cflags` -g -O0
LDADD  := `pkg-config gtk+-2.0 --libs`
CC     := gcc
CXX    := g++

all: brot2

brot2 : $(OBJS)
	  $(CXX) -o $@ $(OBJS) $(LDADD)

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

clean:
	rm -f $(OBJS) $(DEPS)

include $(DEPS)
