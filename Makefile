PROGRAMS=photofinish

CFLAGS += -Wall -Iinclude -g -fopenmp
CXXFLAGS += -Wall -std=c++11 -Iinclude -g -fopenmp

INCLUDES = $(wildcard include/*.hh)
PROG_OBJS = $(patsubst %.cc,%.o,$(wildcard *.cc))
LIB_OBJS = $(patsubst %.cc,%.o,$(wildcard lib/*.cc))

LDFLAGS=
LIBS = -lm -lstdc++ -lpng -llcms2 -lexiv2 -lgomp

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
