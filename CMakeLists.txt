cmake_minimum_required (VERSION 2.6)

find_package(PkgConfig REQUIRED)

pkg_check_modules(gstreamer REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0 gstreamer-allocators-1.0)

project(gstreamer-test-ayoda)
add_library(ayodagst SHARED pool.c pool.h sink.c sink.h plugin.c)
target_link_libraries(ayodagst ${gstreamer_LIBRARIES})
target_include_directories(ayodagst PUBLIC ${gstreamer_INCLUDE_DIRS})
target_compile_options(ayodagst PUBLIC ${gstreamer_CFLAGS_OTHER})
