cmake_minimum_required(VERSION 3.26)
project(mallocLab C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")

#add_executable(mallocLab mm.c memlib.c)
add_executable(mallocLab mm.c memlib.c mdriver.c fsecs.c ftimer.c)
