cmake_minimum_required(VERSION 3.26)
project(mallocLab C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE Release) # Add this line

add_executable(mallocLab mm.c memlib.c mdriver.c fsecs.c ftimer.c)

target_compile_options(mallocLab PRIVATE -O3)  # Add this line
