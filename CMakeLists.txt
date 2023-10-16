cmake_minimum_required(VERSION 3.26)
project(mallocLab C)

set(CMAKE_C_STANDARD 99)

add_executable(mallocLab mm.c memlib.c)
#add_executable(mallocLab mm.c memlib.c mdriver.c fsecs.c ftimer.c)
