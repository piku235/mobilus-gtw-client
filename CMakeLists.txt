cmake_minimum_required(VERSION 3.20)
project(mobilus_gtw_client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED IMPORTED_TARGET libmosquitto)

add_subdirectory(include/jungi/mobilus_gtw_client/proto)
add_subdirectory(src/crypto)
add_subdirectory(third_party/protobuf)
add_subdirectory(tools/mobcli)

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
  src/io/BlockingClientWatcher.cpp
  src/ExponentialBackoff.cpp
  src/ProtoUtils.cpp
  src/ClientId.cpp
  src/Envelope.cpp
  src/SelectCondition.cpp
  src/MqttMobilusGtwClient.cpp
  src/MqttMobilusGtwClientImpl.cpp
)

set_target_properties(mobilus_gtw_client PROPERTIES OUTPUT_NAME "mobgtw")

target_link_libraries(mobilus_gtw_client
  PUBLIC
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
