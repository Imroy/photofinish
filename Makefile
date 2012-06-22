PROGRAMS=photofinish

CFLAGS += -Wall -Iinclude -g -fopenmp
CXXFLAGS += -Wall -std=c++11 -Iinclude -g -fopenmp

INCLUDES = $(wildcard include/*.hh)
OBJS = $(patsubst %.cc,%.o,$(wildcard *.cc)) $(patsubst %.cc,%.o,$(wildcard lib/*.cc))
CLASSES = $(patsubst %.cc,%.o,$(wildcard lib/*.cc))

LDFLAGS=
LIBS = -lm -lstdc++ -lpng -llcms2 -lexiv2 -lgomp

all: $(OBJS) $(PROGRAMS)

clean:
	@rm -fv $(OBJS) $(PROGRAMS)

$(PROGRAMS): %: %.o $(CLASSES)
	$(CXX) $(LDFLAGS) $(LIBS) $^ -o $@

%.o: %.cc $(INCLUDES)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CLASSES): %.o: %.cc $(INCLUDES)
	$(CXX) $(CXXFLAGS) -c $< -o $@

depend:
	touch .depend
	makedepend -f .depend $(INCLUDES) *.cc 2> /dev/null

ifneq ($(wildcard .depend),)
include .depend
endif
