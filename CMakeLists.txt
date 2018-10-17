cmake_minimum_required(VERSION 3.12)

set(CMAKE_BUILD_TYPE Debug)
project(profile_lxd)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -fstrict-aliasing -pedantic")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # not sure cmake was a good idea
subdirs(src)
