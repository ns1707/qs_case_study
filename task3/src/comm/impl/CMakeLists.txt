add_library(communications STATIC
  communication_base.cpp
  communication_builder.cpp
  udp.cpp
)

target_include_directories(communications
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(communications
  PUBLIC comm_interface
  PUBLIC utils
)
