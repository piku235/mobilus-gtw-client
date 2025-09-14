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
    ./configure --with-pic --disable-shared --enable-static --prefix=${CMAKE_BINARY_DIR}/host
  BUILD_IN_SOURCE 1
  BUILD_COMMAND make CXXFLAGS=-w -j${NCPU}
  INSTALL_COMMAND make install
  BUILD_BYPRODUCTS
    "${CMAKE_BINARY_DIR}/host/bin/protoc"
    "${CMAKE_BINARY_DIR}/host/lib/libprotobuf-lite.a"
)

if(CMAKE_CROSSCOMPILING)
  ExternalProject_Add(
    protobuf_target
    DEPENDS protobuf_host
    URL ${PROTOBUF_URL}
    UPDATE_DISCONNECTED 1
    CONFIGURE_COMMAND
      autoreconf -f -i -Wall,no-obsolete &&
      ./configure --with-pic --disable-shared --enable-static --host=${CMAKE_SYSTEM_PROCESSOR}-linux-gnu --with-protoc=${PROTOC} --prefix=${CMAKE_BINARY_DIR}/target
    BUILD_IN_SOURCE 1
    BUILD_COMMAND make CXXFLAGS=-w -j${NCPU}
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS
      "${CMAKE_BINARY_DIR}/target/lib/libprotobuf-lite.a"
  )
endif()

add_library(protobuf-lite STATIC IMPORTED GLOBAL)

if(CMAKE_CROSSCOMPILING)
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
