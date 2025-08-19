add_library(mocks OBJECT
  MockMqttMobilusService.cpp
  MockMqttMobilusActor.cpp
  ${TEST_SOURCES}
)

target_include_directories(mocks
  PUBLIC
    ../
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(mocks
  PRIVATE
    io
    crypto
    mobilus_gtw_client
)
