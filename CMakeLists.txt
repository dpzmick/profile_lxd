cmake_minimum_required(VERSION 3.12)
project(profile_lxd)

# get some system info, probably breaks in some places
# not sure this is really any better than using make (is scons better?)
execute_process(COMMAND getconf LEVEL1_DCACHE_LINESIZE OUTPUT_VARIABLE CACHELINE)
string(STRIP ${CACHELINE} CACHELINE)

# compiler flags
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -fstrict-aliasing -pedantic -march=native -mavx2")
add_definitions(-DCACHELINE=${CACHELINE}ul)
add_definitions(-D_GNU_SOURCE)

# c++ compiler flags, for catch tests
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# I think the add_definitions will percolate down to here?

# cmake garbage
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set(THREADS_PREFER_PTHREAD_FLAG ON)

# find thread library
find_package(Threads REQUIRED)

# create an so for testing in python/julia
add_library(lxd SHARED
    src/additive_square.c
    src/envelope.c
)

# files used in both executables
set(COMMON_FILES
    src/additive_square.c
    src/envelope.c)

# app-specific code
add_executable(profile_lxd
    src/main.c
    src/app.c
    src/disk_thread.c
    ${COMMON_FILES}
)
target_link_libraries(profile_lxd fftw3f)
target_link_libraries(profile_lxd m)
target_link_libraries(profile_lxd jack)
target_link_libraries(profile_lxd Threads::Threads)

# test specific code
add_executable(catch_tests
    src/unit/catch_main.cpp
    src/unit/envelope.cpp
    ${COMMON_FILES}
)

# additional compiler flags which must be specified after the targets are all
# defined

if(CMAKE_BUILD_TYPE STREQUAL "Release")
set_property(TARGET profile_lxd PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
endif()
