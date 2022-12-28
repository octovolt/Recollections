cmake_minimum_required(VERSION 3.14)
project(Recollections_tests)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)

include(FetchContent)
FetchContent_Declare(
  googletest
  DOWNLOAD_EXTRACT_TIMESTAMP
  URL https://github.com/google/googletest/archive/3fa7f983c69f780378b4d1ad44d36030ca951ba6.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  hello_test
  hello_test.cc
  Utils_tests
  Utils_tests.cc
)
target_link_libraries(
  hello_test
  Utils_tests
  ../libraries/ArduinoJson.h
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(
  hello_test
  Utils_tests
)
