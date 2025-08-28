add_library(io OBJECT
  TestClientWatcher.cpp
)

target_include_directories(io
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(io
  PRIVATE
    mobilus_gtw_client
)
