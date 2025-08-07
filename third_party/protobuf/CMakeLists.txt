include(ExternalProject)

ExternalProject_Add(
  protobuf_build
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG v2.6.1
  UPDATE_DISCONNECTED 1
  CONFIGURE_COMMAND
    autoreconf -f -i -Wall,no-obsolete &&
    ./configure --with-pic --disable-shared --enable-static --prefix=${CMAKE_BINARY_DIR}
  BUILD_IN_SOURCE 1
  BUILD_COMMAND make CXXFLAGS=-w -j$(nproc)
  INSTALL_COMMAND make install
  BUILD_BYPRODUCTS
    "${CMAKE_BINARY_DIR}/lib/libprotobuf-lite.a"
    "${CMAKE_BINARY_DIR}/bin/protoc"
)
ExternalProject_Get_Property(protobuf_build install_dir)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include")

add_library(protobuf-lite STATIC IMPORTED GLOBAL)
add_dependencies(protobuf-lite protobuf_build)
set_target_properties(protobuf-lite PROPERTIES
  IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/lib/libprotobuf-lite.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/include"
)
