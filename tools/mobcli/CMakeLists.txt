set(CMAKE_CXX_STANDARD 17)
set(MOBILUS_CA_FILE "${PROJECT_SOURCE_DIR}/certs/mobilelabs_mobilus_ca.crt")

include(FetchContent)

FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG v3.3.1
)
FetchContent_MakeAvailable(cxxopts)

configure_file(
  include/config.h.in
  "${CMAKE_CURRENT_BINARY_DIR}/include/config.h"
  @ONLY
)

add_executable(mobcli
  src/main.cpp
  src/App.cpp
  src/mobilus_gtw_client/StderrLogger.cpp
  src/commands/ClientCommonCommand.cpp
  src/commands/CallEventsCommand.cpp
  src/commands/CurrentStateCommand.cpp
  src/commands/DeviceListCommand.cpp
  src/commands/DeviceSettingsCommand.cpp
  src/commands/NetworkSettingsCommand.cpp
  src/commands/SessionCommand.cpp
  src/commands/SubscribeCommand.cpp
)

target_include_directories(mobcli 
  PRIVATE
    src
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

target_link_libraries(mobcli
  PRIVATE
    cxxopts
    mobilus_gtw_client
)
