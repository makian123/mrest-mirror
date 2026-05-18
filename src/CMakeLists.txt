cmake_minimum_required (VERSION 3.19)

project("MREST")

file(GLOB_RECURSE SOURCES *.cpp)

add_library(MREST ${SOURCES})
target_link_libraries(MREST PRIVATE glaze::glaze)
target_include_directories(MREST PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/mrest ${asio_SOURCE_DIR}/asio/include)