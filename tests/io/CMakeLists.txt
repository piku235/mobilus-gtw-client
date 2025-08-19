add_library(io OBJECT
  TestSocketWatcher.cpp
)

target_link_libraries(io
  PRIVATE
    mobilus_gtw_client
)
