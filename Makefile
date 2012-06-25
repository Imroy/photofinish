PROGRAMS=photofinish

#PRECISION=double
PRECISION=float

# Libraries with pkg-config data
PKGS=libpng lcms2 exiv2

COMMON_FLAGS = -Wall -Iinclude `pkg-config --cflags $(PKGS)` -fopenmp -g -DSAMPLE=$(PRECISION)
CFLAGS += $(COMMON_FLAGS)
CXXFLAGS += -std=c++11 $(COMMON_FLAGS)

INCLUDES = $(wildcard include/*.hh)
PROG_OBJS = $(patsubst %.cc,%.o,$(wildcard *.cc))
LIB_OBJS = $(patsubst %.cc,%.o,$(wildcard lib/*.cc))

LIBS = -lm -lstdc++ -lgomp `pkg-config --libs $(PKGS)` -ljpeg

all: $(PROGRAMS)

clean:
	@rm -fv $(PROG_OBJS) $(LIB_OBJS) $(PROGRAMS)

$(PROGRAMS): %: %.o $(LIB_OBJS)
	$(CXX) $(LDFLAGS) $(LIBS) $^ -o $@

%.o: %.cc $(INCLUDES)
	$(CXX) $(CXXFLAGS) -c $< -o $@

depend:
	touch .depend
	makedepend -f .depend -I include $(INCLUDES) *.cc lib/*.cc 2> /dev/null

ifneq ($(wildcard .depend),)
include .depend
endif
