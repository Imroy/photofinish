PROGRAMS=photofinish process_scans

#PRECISION=double
PRECISION=float

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

# Libraries with pkg-config data
PKGS=libpng lcms2 exiv2 yaml-cpp libtiff-4

COMMON_FLAGS = -Wall -Iinclude `pkg-config --cflags $(PKGS)` -fopenmp -g -DSAMPLE=$(PRECISION) -finput-charset=UTF-8
CFLAGS += $(COMMON_FLAGS)
CXXFLAGS += -std=c++11 $(COMMON_FLAGS)

PROG_OBJS = $(patsubst %.cc,%.o,$(wildcard *.cc))
LIB_OBJS = $(patsubst %.cc,%.o,$(wildcard lib/*.cc))

LIBS = -lm -lstdc++ -lgomp `pkg-config --libs $(PKGS)` -ljpeg -ltiffxx -lopenjpeg -lboost_filesystem-mt -lboost_system-mt -lboost_program_options-mt

all: $(PROGRAMS)

clean:
	@rm -fv $(PROG_OBJS) $(LIB_OBJS) $(PROGRAMS)

install: $(PROGRAMS)
	install -t $(BINDIR) $(PROGRAMS)

$(PROGRAMS): %: $(LIB_OBJS) %.o
	$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

depend:
	touch .depend
	makedepend -f .depend -I include *.cc lib/*.cc 2> /dev/null

ifneq ($(wildcard .depend),)
include .depend
endif
