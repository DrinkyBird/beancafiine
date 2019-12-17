cmake_minimum_required(VERSION 3.16)
project(beancafiine C)

set(CMAKE_C_STANDARD 99)

find_package(Threads REQUIRED)

add_executable(beancafiine main.c connection.c connection.h cafiine.h byteorder.h beandef.h files.c files.h stream.c stream.h)

target_link_libraries(beancafiine ${CMAKE_THREAD_LIBS_INIT})