include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)
# googletest installs itself by default, so `cmake --install` of a C++ build
# put gtest and gmock headers, libraries and CMake configs in the user's prefix
# next to oedesalt.
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
# googletest otherwise builds against the static MSVC runtime, which cannot link
# with oedesalt and the OpenEye SDK, both of which use the DLL runtime (/MD).
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(oedesalt_tests
    test_desalter.cpp
)

target_link_libraries(oedesalt_tests
    PRIVATE
        oedesalt
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(oedesalt_tests)
