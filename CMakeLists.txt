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

add_library(ArtLib INTERFACE)
target_include_directories(
    ArtLib
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

add_library(SteroidsLib STATIC ./include/steroids.hpp)
set_target_properties(SteroidsLib PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(SteroidsLib PUBLIC ./include)
target_compile_options(SteroidsLib PUBLIC ${BITHACKING_COMPILE_FLAGS})
target_link_libraries(SteroidsLib PRIVATE ArtLib)


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
    #add_subdirectory(bench)
endif ()

