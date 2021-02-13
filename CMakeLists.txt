cmake_minimum_required(VERSION 3.10)
project(PhotoFinish)

set (CMAKE_CXX_STANDARD 17)
set(COMMON_FLAGS "-Wall -fopenmp -finput-charset=UTF-8 -Wformat -Wformat-security -Werror=format-security -D_FORTIFY_SOURCE=2 -fstack-protector --param ssp-buffer-size=4 -fPIE -pie")

# LTO
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} ${CMAKE_CXX_FLAGS} -Wl,-z,relro -Wl,-z,now -fPIE -pie -flto")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -flto -fuse-linker-plugin")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -flto -fuse-linker-plugin")


find_package(Boost
	REQUIRED
	filesystem
	system
	program_options
)

# Pull in support for pkg-config
find_package(PkgConfig REQUIRED)

pkg_check_modules(LCMS2 REQUIRED lcms2)
pkg_check_modules(EXIV2 REQUIRED exiv2)
pkg_check_modules(YAMLCPP REQUIRED yaml-cpp)

set(CORE_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} ${LCMS2_INCLUDE_DIRS} ${EXIV2_INCLUDE_DIRS} ${YAMLCPP_INCLUDE_DIRS})
set(CORE_CFLAGS ${LCMS2_CFLAGS} ${EXIV2_CFLAGS} ${YAMLCPP_CFLAGS} ${YAMLCPP_CFLAGS})
set(CORE_LIBRARIES ${Boost_LIBRARIES} ${LCMS2_LIBRARIES} ${EXIV2_LIBRARIES} ${YAMLCPP_LIBRARIES})

file(GLOB CORE_FILES
        include/*.hh
        include/*.tcc
        lib/*.cc
)

pkg_check_modules(PNG libpng>=1.6)
pkg_check_modules(JPEG libjpeg)
pkg_check_modules(TIFF libtiff-4)
pkg_check_modules(JP2 libopenjp2)
pkg_check_modules(WEBP libwebp)
# TODO JXR?
# TODO FLIF?
pkg_check_modules(HEIF libheif)
pkg_check_modules(JXL libjxl)
pkg_check_modules(JXL_THREADS libjxl_threads)

set(FORMATS_FILES lib/formats/SOLwriter.cc)
set(FORMATS_INCLUDE_DIRS lib/formats)
set(FORMATS_CFLAGS)
set(FORMATS_LIBRARIES)

if (PNG_FOUND)
	file(GLOB PNG_FILES
		lib/formats/PNG*.hh
		lib/formats/PNG*.cc
	)
	set(FORMATS_FILES ${FORMATS_FILES} ${PNG_FILES})
	set(FORMATS_INCLUDE_DIRS ${FORMATS_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
	set(FORMATS_CFLAGS ${FORMATS_CFLAGS} -DHAZ_PNG ${PNG_CFLAGS})
	set(FORMATS_LIBRARIES ${FORMATS_LIBRARIES} ${PNG_LIBRARIES})
endif()
if (JPEG_FOUND)
	file(GLOB JPEG_FILES
		lib/formats/JPEG*.hh
		lib/formats/JPEG*.cc
	)
	set(FORMATS_FILES ${FORMATS_FILES} ${JPEG_FILES})
	set(FORMATS_INCLUDE_DIRS ${FORMATS_INCLUDE_DIRS} ${JPEG_INCLUDE_DIRS})
	set(FORMATS_CFLAGS ${FORMATS_CFLAGS} -DHAZ_JPEG ${JPEG_CFLAGS})
	set(FORMATS_LIBRARIES ${FORMATS_LIBRARIES} ${JPEG_LIBRARIES})
endif()
if (TIFF_FOUND)
	file(GLOB TIFF_FILES
		lib/formats/TIFF*.hh
		lib/formats/TIFF*.cc
	)
	set(FORMATS_FILES ${FORMATS_FILES} ${TIFF_FILES})
	set(FORMATS_INCLUDE_DIRS ${FORMATS_INCLUDE_DIRS} ${TIFF_INCLUDE_DIRS})
	set(FORMATS_CFLAGS ${FORMATS_CFLAGS} -DHAZ_TIFF ${TIFF_CFLAGS})
	set(FORMATS_LIBRARIES ${FORMATS_LIBRARIES} ${TIFF_LIBRARIES} -ltiffxx)
endif()
if (JP2_FOUND)
	file(GLOB JP2_FILES
		lib/formats/JP2*.hh
		lib/formats/JP2*.cc
	)
	set(FORMATS_FILES ${FORMATS_FILES} ${JP2_FILES})
	set(FORMATS_INCLUDE_DIRS ${FORMATS_INCLUDE_DIRS} ${JP2_INCLUDE_DIRS})
	set(FORMATS_CFLAGS ${FORMATS_CFLAGS} -DHAZ_JP2 ${JP2_CFLAGS})
	set(FORMATS_LIBRARIES ${FORMATS_LIBRARIES} ${JP2_LIBRARIES})
endif()
if (WEBP_FOUND)
	file(GLOB WEBP_FILES
		lib/formats/WebP*.hh
		lib/formats/WebP*.cc
	)
	set(FORMATS_FILES ${FORMATS_FILES} ${WEBP_FILES})
	set(FORMATS_INCLUDE_DIRS ${FORMATS_INCLUDE_DIRS} ${WEBP_INCLUDE_DIRS})
	set(FORMATS_CFLAGS ${FORMATS_CFLAGS} -DHAZ_WEBP ${WEBP_CFLAGS})
	set(FORMATS_LIBRARIES ${FORMATS_LIBRARIES} ${WEBP_LIBRARIES})
endif()
# TODO JXR?
#if (FLIF_FOUND)
	file(GLOB FLIF_FILES
		lib/formats/FLIF*.hh
		lib/formats/FLIF*.cc
	)
	set(FORMATS_FILES ${FORMATS_FILES} ${FLIF_FILES})
	set(FORMATS_INCLUDE_DIRS ${FORMATS_INCLUDE_DIRS} /usr/local/include)
	set(FORMATS_CFLAGS ${FORMATS_CFLAGS} -DHAZ_FLIF)
	set(FORMATS_LIBRARIES ${FORMATS_LIBRARIES} -lflif)
#endif()
if (HEIF_FOUND)
	file(GLOB HEIF_FILES
		lib/formats/HEIF*.hh
		lib/formats/HEIF*.cc
	)
	set(FORMATS_FILES ${FORMATS_FILES} ${HEIF_FILES})
	set(FORMATS_INCLUDE_DIRS ${FORMATS_INCLUDE_DIRS} ${HEIF_INCLUDE_DIRS})
	set(FORMATS_CFLAGS ${FORMATS_CFLAGS} -DHAZ_HEIF ${HEIF_CFLAGS})
	set(FORMATS_LIBRARIES ${FORMATS_LIBRARIES} ${HEIF_LIBRARIES})
endif()
if (JXL_FOUND AND JXL_THREADS_FOUND)
	file(GLOB JXL_FILES
		lib/formats/JXL*.hh
		lib/formats/JXL*.cc
	)
	set(FORMATS_FILES ${FORMATS_FILES} ${JXL_FILES})
	set(FORMATS_INCLUDE_DIRS ${FORMATS_INCLUDE_DIRS} ${JXL_INCLUDE_DIRS} ${JXL_THREADS_INCLUDE_DIRS})
	set(FORMATS_CFLAGS ${FORMATS_CFLAGS} -DHAZ_JXL ${JXL_CFLAGS} ${JXL_THREADS_CFLAGS})
	set(FORMATS_LIBRARIES ${FORMATS_LIBRARIES} ${JXL_LIBRARIES} ${JXL_THREADS_LIBRARIES})
endif()

add_library(pf
	${CORE_FILES}
	${FORMATS_FILES}
)
target_include_directories(pf
	PUBLIC
	include
	lib/formats
	${CORE_INCLUDE_DIRS}
	${FORMATS_INCLUDE_DIRS}
)
target_compile_options(pf
	PUBLIC
	${CORE_CFLAGS}
	${FORMATS_CFLAGS}
)

add_executable(photofinish
        photofinish.cc
)
add_executable(process_scans
        process_scans.cc
)

target_include_directories(photofinish
	PUBLIC
	${CORE_INCLUDE_DIRS}
	${FORMATS_INCLUDE_DIRS}
)
target_compile_options(photofinish
	PUBLIC
	${CORE_CFLAGS}
	${FORMATS_CFLAGS}
)
target_link_libraries(photofinish
	pf
	${CORE_LIBRARIES}
	${FORMATS_LIBRARIES}
)

target_include_directories(process_scans
	PUBLIC
	${CORE_INCLUDE_DIRS}
	${FORMATS_INCLUDE_DIRS}
)
target_compile_options(process_scans
	PUBLIC
	${CORE_CFLAGS}
	${FORMATS_CFLAGS}
)
target_link_libraries(process_scans
	pf
	${CORE_LIBRARIES}
	${FORMATS_LIBRARIES}
)

install(TARGETS
	photofinish process_scans
	DESTINATION /usr/local/bin
)
