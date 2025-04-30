set(SRCS
  HTTPServer.cpp
  ControllerCommands.cpp
  MemWatch.cpp
  SaveState.cpp
)

set(HEADERS
  HTTPServer.h
  ControllerCommands.h
  MemWatch.h
  SaveState.h
)

add_library(dolphin-ipc STATIC ${SRCS} ${HEADERS})
target_include_directories(dolphin-ipc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(dolphin-ipc
PUBLIC
  common
  httplib

PRIVATE
  nlohmann_json
)
