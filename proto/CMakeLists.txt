set(PROTOC "${CMAKE_BINARY_DIR}/host/bin/protoc")
file(GLOB PROTO_FILES *.proto)

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/jungi/mobilus_gtw_client/proto)

foreach(proto ${PROTO_FILES})
  get_filename_component(filename ${proto} NAME_WE)
  list(APPEND PROTOS "${GENERATED_DIR}/${filename}.pb.h")
  list(APPEND PROTOS "${GENERATED_DIR}/${filename}.pb.cc")
endforeach()

add_custom_command(
  OUTPUT ${PROTOS}
  COMMAND ${PROTOC} -I ${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=${GENERATED_DIR} ${PROTO_FILES}
  DEPENDS ${PROTOC} ${PROTO_FILES}
  COMMENT "Generating mobilus protos"
)

add_library(protos OBJECT ${PROTOS})

target_link_libraries(protos
  PUBLIC protobuf-lite
)

target_include_directories(protos
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)
