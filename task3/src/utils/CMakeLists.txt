add_library(utils INTERFACE)

target_sources(utils INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/socket_utils.hpp
)

target_include_directories(utils INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
)
