cmake_minimum_required(VERSION 3.12)
project(profile_lxd)

# get some system info, probably breaks in some places
# not sure this is really any better than using make (is scons better?)
execute_process(COMMAND getconf LEVEL1_DCACHE_LINESIZE OUTPUT_VARIABLE CACHELINE)

# compiler flags
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -fstrict-aliasing -pedantic -march=native -mavx2")
add_definitions(-DCACHELINE=${CACHELINE})

# cmake garbage
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

# add the sources
add_executable(profile_lxd src/main.c src/app.c src/additive_square.c)
set_property(TARGET profile_lxd PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
target_link_libraries(profile_lxd m)
target_link_libraries(profile_lxd jack)
