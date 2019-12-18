cmake_minimum_required(VERSION 3.2)
project(beancafiine C)

set(CMAKE_C_STANDARD 99)

find_package(Threads REQUIRED)

set (BEANCAFIINE_SOURCES
    main.c
    connection.c
    connection.h
    cafiine.h
    byteorder.h
    beandef.h
    files.c
    files.h
    stream.c
    stream.h
)

add_executable(beancafiine ${BEANCAFIINE_SOURCES})

target_link_libraries(beancafiine ${CMAKE_THREAD_LIBS_INIT})

if (WIN32)
    target_link_libraries(beancafiine ws2_32)
endif ()