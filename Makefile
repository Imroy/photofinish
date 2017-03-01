HAZ_PNG = 1
HAZ_JPEG = 1
HAZ_TIFF = 1
HAZ_JP2 = 1
HAZ_WEBP = 1
HAZ_JXR = 1

PROGRAMS = photofinish process_scans

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

# Libraries with pkg-config data
PKGS = lcms2 exiv2 yaml-cpp

COMMON_FLAGS = -Wall -Iinclude -fopenmp -finput-charset=UTF-8 -Wformat -Wformat-security -Werror=format-security -D_FORTIFY_SOURCE=2 -fstack-protector --param ssp-buffer-size=4 -fPIE -pie
LIBS = -lm -lstdc++ -lgomp -lboost_filesystem -lboost_system -lboost_program_options
LIB_OBJS = $(patsubst %.cc,%.o, $(wildcard lib/*.cc)) lib/formats/SOLwriter.o

ifeq ($(HAZ_PNG), 1)
COMMON_FLAGS += -DHAZ_PNG
PKGS += libpng16
LIB_OBJS += $(patsubst %.cc,%.o, $(wildcard lib/formats/PNG*.cc))
endif
ifeq ($(HAZ_JPEG), 1)
COMMON_FLAGS += -DHAZ_JPEG
LIBS += -ljpeg
LIB_OBJS += $(patsubst %.cc,%.o, $(wildcard lib/formats/JPEG*.cc))
endif
ifeq ($(HAZ_TIFF), 1)
COMMON_FLAGS += -DHAZ_TIFF
PKGS += libtiff-4
LIBS += -ltiffxx
LIB_OBJS += $(patsubst %.cc,%.o, $(wildcard lib/formats/TIFF*.cc))
endif
ifeq ($(HAZ_JP2), 1)
COMMON_FLAGS += -DHAZ_JP2
PKGS += libopenjpeg1
LIB_OBJS += $(patsubst %.cc,%.o, $(wildcard lib/formats/JP2*.cc))
endif
ifeq ($(HAZ_WEBP), 1)
COMMON_FLAGS += -DHAZ_WEBP
PKGS += libwebp
LIB_OBJS += $(patsubst %.cc,%.o, $(wildcard lib/formats/WebP*.cc))
endif
ifeq ($(HAZ_JXR), 1)
COMMON_FLAGS += -DHAZ_JXR
LIBS += -ljpegxr -ljxrglue
LIB_OBJS += $(patsubst %.cc,%.o, $(wildcard lib/formats/JXR*.cc))
endif

COMMON_FLAGS += `pkg-config --cflags $(PKGS)`
CFLAGS += $(COMMON_FLAGS)
CXXFLAGS += -std=c++11 $(COMMON_FLAGS)
LDFLAGS += -Wl,-z,relro -Wl,-z,now -fPIE -pie
LIBS += `pkg-config --libs $(PKGS)`

PROG_OBJS = $(patsubst %.cc,%.o,$(wildcard *.cc))


all: $(PROGRAMS)

clean:
	@rm -fv .depend *.o lib/*.o lib/formats/*.o $(PROGRAMS)

install: $(PROGRAMS)
	install -t $(BINDIR) $(PROGRAMS)

$(PROGRAMS): %: $(LIB_OBJS) %.o
	$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

# A special case for compiling with jxrlib
ifeq ($(HAZ_JXR), 1)
$(patsubst %.cc,%.o, $(wildcard lib/formats/JXR*.cc)): %.o: %.cc
	$(CXX) $(CXXFLAGS) -I/usr/include/jxrlib -D__ANSI__ -c $< -o $@
endif

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

depend:
	touch .depend
	makedepend -f .depend -I include -I lib/formats *.cc lib/*.cc lib/formats/*.cc 2> /dev/null

ifneq ($(wildcard .depend),)
include .depend
endif
