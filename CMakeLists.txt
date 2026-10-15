cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

project(oedesalt VERSION 0.1.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(OEDESALT_BUILD_TESTS "Build tests" ON)
option(OEDESALT_BUILD_PYTHON "Build Python bindings" ON)
option(OEDESALT_UNIVERSAL2 "Build universal2 binary for macOS" OFF)
option(OEDESALT_USE_STABLE_ABI "Use Python stable ABI" ON)

if(APPLE AND OEDESALT_UNIVERSAL2)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures" FORCE)
elseif(APPLE)
    unset(CMAKE_OSX_ARCHITECTURES CACHE)
endif()

if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

# Dependencies
include(FetchContent)

FetchContent_Declare(
    cmake_openeye
    GIT_REPOSITORY https://github.com/scott-arne/cmake-openeye.git
    GIT_TAG v1.2.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cmake_openeye)
list(APPEND CMAKE_MODULE_PATH "${cmake_openeye_SOURCE_DIR}")

# The interpreter is always needed for the embedded-pattern codegen step.
find_package(Python3 COMPONENTS Interpreter REQUIRED)

if(OEDESALT_BUILD_PYTHON)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    # SWIG 4.4.0 picks Python 3.13-only C API calls from the header version
    # instead of Py_LIMITED_API, so its stable-ABI wrapper does not compile
    # against Python 3.13+ headers. 4.4.1 fixed it (swig/swig#3283).
    if(OEDESALT_USE_STABLE_ABI)
        find_package(SWIG 4.4.1)
    else()
        find_package(SWIG 4.0)
    endif()
    # The bindings below are skipped when SWIG is missing, and wheel builds
    # force OEDESALT_BUILD_PYTHON ON, so a too-old SWIG would otherwise produce a
    # wheel with no extension. Fail here instead, as oemmpa does.
    if(NOT SWIG_FOUND)
        message(FATAL_ERROR
            "OEDESALT_BUILD_PYTHON is ON but no suitable SWIG was found "
            "(the stable-ABI build needs 4.4.1 or newer; SWIG 4.4.0, which "
            "Ubuntu 26.04 ships, is too old). Install a newer SWIG (pip install "
            "swig==4.5.1) and pass -DSWIG_EXECUTABLE=<path>, deleting "
            "CMakeCache.txt first if an older SWIG is cached, or configure "
            "with -DOEDESALT_BUILD_PYTHON=OFF for a C++-only build.")
    endif()
endif()

if(OPENEYE_USE_SHARED)
    find_package(OpenEyePython REQUIRED)
endif()

find_package(OpenEye REQUIRED)

# Library
# Embed the default salt/solvent SMARTS into the library at build time. The
# .smarts files are the single source of truth; this generates a .cpp of raw
# string literals that WithBundledPatterns() parses. Regenerates when a data
# file (or the generator) changes.
set(OEDESALT_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(OEDESALT_BUNDLED_PATTERNS_CPP ${OEDESALT_GENERATED_DIR}/bundled_patterns.cpp)
set(OEDESALT_SALTS_SMARTS ${CMAKE_CURRENT_SOURCE_DIR}/python/oedesalt/data/salts.smarts)
set(OEDESALT_SOLVENTS_SMARTS ${CMAKE_CURRENT_SOURCE_DIR}/python/oedesalt/data/solvents.smarts)

add_custom_command(
    OUTPUT ${OEDESALT_BUNDLED_PATTERNS_CPP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OEDESALT_GENERATED_DIR}
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_bundled_patterns.py
            --salts ${OEDESALT_SALTS_SMARTS}
            --solvents ${OEDESALT_SOLVENTS_SMARTS}
            --output ${OEDESALT_BUNDLED_PATTERNS_CPP}
    DEPENDS
        ${OEDESALT_SALTS_SMARTS}
        ${OEDESALT_SOLVENTS_SMARTS}
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_bundled_patterns.py
    COMMENT "Generating embedded salt/solvent patterns"
    VERBATIM
)

set(OEDESALT_SOURCES
    src/Desalter.cpp
    ${OEDESALT_BUNDLED_PATTERNS_CPP}
)

add_library(oedesalt ${OEDESALT_SOURCES})

set_target_properties(oedesalt PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(oedesalt
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(oedesalt
    PUBLIC
        OpenEye::OEChem
)

# Tests
if(OEDESALT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Python bindings
if(OEDESALT_BUILD_PYTHON AND SWIG_FOUND)
    add_subdirectory(swig)
endif()

# Install
install(TARGETS oedesalt
    EXPORT oedesalt-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/oedesalt DESTINATION include)

install(EXPORT oedesalt-targets
    FILE OEDESALTTargets.cmake
    NAMESPACE OEDESALT::
    DESTINATION lib/cmake/oedesalt
)
