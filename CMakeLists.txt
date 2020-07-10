
cmake_minimum_required(VERSION 2.8)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
endif()

set(CMAKE_PROJECT_DESCRIPTION "Hello")
set(CMAKE_PROJECT_FULL_NAME "Projet Name")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(Bulletin_Board_Server CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

#include_directories(inc)

# Packaging
#ADD_CUSTOM_TARGET(packaging
#    COMMAND rm -rf build
#    COMMAND mkdir build
#    COMMAND cd build
#    COMMAND cmake ${CMAKE_SOURCE_DIR} -DCMAKE_BUILD_TYPE=Release
#    COMMAND make package -j${NB_PROCESSOR}
#    COMMAND cp build/*.rpm build/*.deb . || true
#    COMMAND rm -rf build
#    COMMAND mkdir build

# pkg_config
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/Bulletin_Board_Server.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/Bulletin_Board_Server.pc @ONLY
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/Bulletin Board Server.pc
    DESTINATION lib/pkgconfig/
)

# Testing
enable_testing()

add_test(
    NAME Test
    COMMAND Tests-Bulletin Board Server
)

# Coverage
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CodeCoverage)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    SETUP_TARGET_FOR_COVERAGE(
        test-coverage
        Tests-Bulletin Board Server
        coverage
    )
endif()



# Packaging
#ADD_CUSTOM_TARGET(packaging
#    COMMAND rm -rf build
#    COMMAND mkdir build
#    COMMAND cd build
#    COMMAND cmake ${CMAKE_SOURCE_DIR} -DCMAKE_BUILD_TYPE=Release
#    COMMAND make package -j${NB_PROCESSOR}
#    COMMAND cp build/*.rpm build/*.deb . || true
#    COMMAND rm -rf build
#    COMMAND mkdir build
#    COMMAND cd build
#    COMMAND cmake ${CMAKE_SOURCE_DIR} -DCMAKE_BUILD_TYPE=Debug
#    COMMAND make package -j${NB_PROCESSOR}
#    COMMAND cp build/*.rpm build/*.deb . || true
#    COMMAND rm -rf build
#    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#)

# Subdirectories
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(doc)

