#PRECISION=double
PRECISION=float

HAZ_PNG = 1
HAZ_JPEG = 1
HAZ_TIFF = 1
HAZ_JP2 = 1

PROGRAMS = photofinish process_scans

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

# Libraries with pkg-config data
PKGS = lcms2 exiv2 yaml-cpp

COMMON_FLAGS = -Wall -Iinclude -fopenmp -DSAMPLE=$(PRECISION) -finput-charset=UTF-8
LIBS = -lm -lstdc++ -lgomp -lboost_filesystem-mt -lboost_system-mt -lboost_program_options-mt
LIB_OBJS = $(patsubst %.cc,%.o, $(wildcard lib/*.cc)) lib/formats/SOLfile.o

ifeq ($(HAZ_PNG), 1)
COMMON_FLAGS += -DHAZ_PNG
PKGS += libpng
LIB_OBJS += lib/formats/PNGfile.o
endif
ifeq ($(HAZ_JPEG), 1)
COMMON_FLAGS += -DHAZ_JPEG
LIBS += -ljpeg
LIB_OBJS += $(patsubst %.cc,%.o, $(wildcard lib/formats/JPEGfile*.cc))
endif
ifeq ($(HAZ_TIFF), 1)
COMMON_FLAGS += -DHAZ_TIFF
PKGS += libtiff-4
LIBS += -ltiffxx
LIB_OBJS += lib/formats/TIFFfile.o
endif
ifeq ($(HAZ_JP2), 1)
COMMON_FLAGS += -DHAZ_JP2
PKGS += libopenjpeg1
LIB_OBJS += lib/formats/JP2file.o
endif

COMMON_FLAGS += `pkg-config --cflags $(PKGS)`
CFLAGS += $(COMMON_FLAGS)
CXXFLAGS += -std=c++11 $(COMMON_FLAGS)
LIBS += `pkg-config --libs $(PKGS)`

PROG_OBJS = $(patsubst %.cc,%.o,$(wildcard *.cc))


all: $(PROGRAMS)

clean:
	@rm -fv *.o lib/*.o lib/formats/*.o $(PROGRAMS)

install: $(PROGRAMS)
	install -t $(BINDIR) $(PROGRAMS)

$(PROGRAMS): %: $(LIB_OBJS) %.o
	$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

depend:
	touch .depend
	makedepend -f .depend -I include *.cc lib/*.cc lib/formats/*.cc 2> /dev/null

ifneq ($(wildcard .depend),)
include .depend
endif
