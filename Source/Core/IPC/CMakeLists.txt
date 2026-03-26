set(SRCS
  HTTPServer.cpp
  ControllerCommands.cpp
  MemWatcher.cpp
  SaveState.cpp
)

set(HEADERS
  HTTPServer.h
  ControllerCommands.h
  MemWatcher.h
  SaveState.h
)

# Find Qt packages required for this module
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_library(dolphin-ipc STATIC ${SRCS} ${HEADERS})
target_include_directories(dolphin-ipc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(dolphin-ipc
PUBLIC
  common
  httplib
  core
  Qt6::Core
  Qt6::Widgets

PRIVATE
  nlohmann_json
)
