# create an interface library for the communication module
add_library(comm_interface INTERFACE)

target_sources(comm_interface INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/comm_types.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/communication_builder.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/iCommunication.hpp
)

target_include_directories(comm_interface INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
) 

