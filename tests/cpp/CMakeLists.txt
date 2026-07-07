include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(oedesalt_tests
    test_oedesalt.cpp
)

target_link_libraries(oedesalt_tests
    PRIVATE
        oedesalt
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(oedesalt_tests)
