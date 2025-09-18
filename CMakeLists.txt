cmake_minimum_required(VERSION 3.25)
project(mobilus_gtw_client VERSION 0.3.1)

cmake_policy(SET CMP0135 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CMakeDependentOption)
include(CTest)
include(FetchContent)

cmake_dependent_option(MOBGTW_BUILD_TESTS "Build mobgtw tests" ON "PROJECT_IS_TOP_LEVEL AND BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING" OFF)

find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED IMPORTED_TARGET libmosquitto)

FetchContent_Declare(
  tl_expected
  URL https://github.com/TartanLlama/expected/archive/refs/tags/v1.3.1.zip
)

set(EXPECTED_BUILD_TESTS OFF)
FetchContent_MakeAvailable(tl_expected)

add_subdirectory(include/jungi/mobilus_gtw_client/proto)
add_subdirectory(src/crypto)
add_subdirectory(third_party/protobuf)

if(MOBGTW_BUILD_TESTS)
  add_subdirectory(tests)
endif()

add_library(common_compiler_warnings INTERFACE)

target_compile_options(common_compiler_warnings
  INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
)

add_library(mobilus_gtw_client
  src/io/SelectEventLoop.cpp
  src/ExponentialBackoff.cpp
  src/TimeUtils.cpp
  src/ProtoUtils.cpp
  src/ClientId.cpp
  src/Envelope.cpp
  src/SelectCondition.cpp
  src/MqttDsn.cpp
  src/MqttMobilusGtwClient.cpp
  src/MqttMobilusGtwClientImpl.cpp
)

set_target_properties(mobilus_gtw_client PROPERTIES OUTPUT_NAME "mobgtw")

target_link_libraries(mobilus_gtw_client
  PUBLIC
    expected
    protos
  PRIVATE
    common_compiler_warnings
    OpenSSL::Crypto
    PkgConfig::MOSQUITTO
    crypto
)

target_include_directories(mobilus_gtw_client
  PUBLIC
    include
  PRIVATE
    src
)
