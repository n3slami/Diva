cmake_minimum_required(VERSION 3.16.3)
project(SNARFOnSteroids
        VERSION 1.0
        HOMEPAGE_URL https://github.com/n3slami/SNARFOnSteroids)

option(BUILD_TESTS "Build the tests" ON)
option(BUILD_EXAMPLES "Build the examples" ON)
option(BUILD_BENCHMARKS "Build the benchmark targets" ON)

set(CMAKE_CXX_STANDARD 17)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
else()
    set(USE_MULTI_THREADED OFF)
endif ()

set(BITHACKING_COMPILE_FLAGS -Ofast -march=native)
#set(BITHACKING_COMPILE_FLAGS -Ofast -mbmi2 -msse4.2 -D__SSE4_2_)

add_library(WormholeLib STATIC ./include/wormhole/kv.c ./include/wormhole/lib.c ./include/wormhole/wh.c
                               ./include/wormhole/kv.h ./include/wormhole/lib.h ./include/wormhole/wh.h)
set_target_properties(WormholeLib PROPERTIES LINKER_LANGUAGE CXX)
target_compile_options(WormholeLib PUBLIC ${BITHACKING_COMPILE_FLAGS} -pthread -fno-stack-protector -flto
                                          -fno-builtin-memcpy -fno-builtin-memmove -fno-builtin-memcmp
                                          -fmax-errors=3 -shared -fPIC)
target_compile_definitions(WormholeLib PUBLIC __x86_64__ NDEBUG)
target_include_directories(
    WormholeLib
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/wormhole>"
    "$<INSTALL_INTERFACE:include/wormhole>"
)

add_library(WormholeIntLib STATIC ./include/wormhole/kv.c ./include/wormhole/lib.c ./include/wormhole/wh_int.c
                                  ./include/wormhole/kv.h ./include/wormhole/lib.h ./include/wormhole/wh_int.h)
set_target_properties(WormholeIntLib PROPERTIES LINKER_LANGUAGE CXX)
target_compile_options(WormholeIntLib PUBLIC ${BITHACKING_COMPILE_FLAGS} -pthread -fno-stack-protector -flto
                                             -fno-builtin-memcpy -fno-builtin-memmove -fno-builtin-memcmp
                                             -fmax-errors=3 -shared -fPIC)
target_compile_definitions(WormholeIntLib PUBLIC __x86_64__ NDEBUG)
target_include_directories(
    WormholeIntLib
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/wormhole>"
    "$<INSTALL_INTERFACE:include/wormhole>"
)

add_library(SteroidsLib STATIC ./include/steroids.hpp)
set_target_properties(SteroidsLib PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(SteroidsLib PUBLIC ./include)
target_compile_options(SteroidsLib PUBLIC ${BITHACKING_COMPILE_FLAGS})
target_link_libraries(SteroidsLib PRIVATE WormholeLib WormholeIntLib)

if (BUILD_TESTS)
    # enable testing 
    include(CTest)
    message(STATUS "Building tests")
    add_subdirectory(tests)
endif ()

if (BUILD_EXAMPLES)
    message(STATUS "Building examples")
    add_subdirectory(examples)
endif ()

if (BUILD_BENCHMARKS)
    message(STATUS "Building benchmarks")
    add_subdirectory(bench)
endif ()
