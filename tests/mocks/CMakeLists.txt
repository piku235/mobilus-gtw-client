find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED IMPORTED_TARGET libmosquitto)

add_library(mocks OBJECT
  MockMqttMobilusActor.cpp
  MockMqttMobilusActorImpl.cpp
)

target_include_directories(mocks
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(mocks
  PRIVATE
    PkgConfig::MOSQUITTO
    mobilus_gtw_client
)
