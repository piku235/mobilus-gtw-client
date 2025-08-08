include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)
add_executable(tests ${TEST_SOURCES})

target_link_libraries(tests
  PRIVATE
    mobilus_gtw_client
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)
