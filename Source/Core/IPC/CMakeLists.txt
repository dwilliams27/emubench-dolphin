set(SRCS
  HTTPServer.cpp
)

set(HEADERS
  HTTPServer.h
)

add_library(dolphin-ipc STATIC ${SRCS} ${HEADERS})
target_include_directories(dolphin-ipc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(dolphin-ipc PRIVATE httplib nlohmann_json)
