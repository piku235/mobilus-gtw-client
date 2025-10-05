include(ExternalProject)
include(ProcessorCount)
ProcessorCount(NCPU)

set(PROTOBUF_URL https://github.com/protocolbuffers/protobuf/archive/refs/tags/v2.6.1.zip)

ExternalProject_Add(
  protobuf_host
  URL ${PROTOBUF_URL}
  UPDATE_DISCONNECTED 1
  CONFIGURE_COMMAND
    autoreconf -f -i -Wall,no-obsolete &&
    ./configure CXXFLAGS=-w --with-pic --disable-shared --enable-static --prefix=${CMAKE_BINARY_DIR}/host
  BUILD_IN_SOURCE 1
  BUILD_COMMAND make -j${NCPU}
  INSTALL_COMMAND make install
  BUILD_BYPRODUCTS
    "${CMAKE_BINARY_DIR}/host/bin/protoc"
    "${CMAKE_BINARY_DIR}/host/lib/libprotobuf-lite.a"
)

add_library(protobuf-lite STATIC IMPORTED GLOBAL)

if(CMAKE_CROSSCOMPILING)
  if(NOT DEFINED CROSS_C_FLAGS)
    set(CROSS_C_FLAGS "")
  endif()
  if(NOT DEFINED CROSS_CXX_FLAGS)
    set(CROSS_CXX_FLAGS "")
  endif()

  get_filename_component(COMPILER_NAME ${CMAKE_C_COMPILER} NAME_WE)
  string(REGEX REPLACE "-gcc$" "" CROSS_HOST ${COMPILER_NAME})
  set(CROSS_CXX_FLAGS "-w ${CROSS_CXX_FLAGS}")

  ExternalProject_Add(
    protobuf_target
    DEPENDS protobuf_host
    URL ${PROTOBUF_URL}
    UPDATE_DISCONNECTED 1
    CONFIGURE_COMMAND
      autoreconf -f -i -Wall,no-obsolete &&
      ./configure CFLAGS=${CROSS_C_FLAGS} CXXFLAGS=${CROSS_CXX_FLAGS} --with-pic --disable-shared --enable-static --host=${CROSS_HOST} --with-protoc=${PROTOC} --prefix=${CMAKE_BINARY_DIR}/target
    BUILD_IN_SOURCE 1
    BUILD_COMMAND make -j${NCPU}
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS
      "${CMAKE_BINARY_DIR}/target/lib/libprotobuf-lite.a"
  )

  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/target/include")
  add_dependencies(protobuf-lite protobuf_target)
  set_target_properties(protobuf-lite PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/target/lib/libprotobuf-lite.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/target/include"
  )
else()
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/host/include")
  add_dependencies(protobuf-lite protobuf_host)
  set_target_properties(protobuf-lite PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/host/lib/libprotobuf-lite.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/host/include"
  )
endif()
