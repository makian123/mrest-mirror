cmake_minimum_required (VERSION 3.19)

project("MREST")

file(GLOB_RECURSE SOURCES *.cpp)

add_library(MREST ${SOURCES})
target_link_libraries(MREST PRIVATE asio glaze::glaze)
target_include_directories(MREST PUBLIC ${CMAKE_SOURCE_DIR}/include/mrest asio)