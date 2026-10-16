# A standalone project that links an installed oedesalt the way README.md's
# "Consuming the library" section tells users to. CI configures it against the
# prefix from `cmake --install`, so a broken install rule, targets file or
# README example fails a build instead of a user's.
#
#   cmake -S tests/consumer -B build-consumer \
#       -DOEDESALT_PREFIX=<install-prefix> -DOPENEYE_ROOT=<sdk>
cmake_minimum_required(VERSION 3.16)
project(oedesalt_consumer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

if(NOT OEDESALT_PREFIX)
    message(FATAL_ERROR "Set -DOEDESALT_PREFIX to the oedesalt install prefix")
endif()

include(FetchContent)
FetchContent_Declare(cmake_openeye
    GIT_REPOSITORY https://github.com/scott-arne/cmake-openeye.git
    GIT_TAG v1.2.1 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(cmake_openeye)
list(APPEND CMAKE_MODULE_PATH "${cmake_openeye_SOURCE_DIR}")
find_package(OpenEye REQUIRED)

include(${OEDESALT_PREFIX}/lib/cmake/oedesalt/OEDESALTTargets.cmake)

add_executable(oedesalt_consumer main.cpp)
target_link_libraries(oedesalt_consumer PRIVATE OEDESALT::oedesalt OpenEye::OEChem)

enable_testing()
add_test(NAME oedesalt_consumer COMMAND oedesalt_consumer)
