cmake_minimum_required (VERSION 3.19)

project("Example")

file(GLOB_RECURSE SOURCES *.cpp)

add_executable(Example ${SOURCES})
target_link_libraries(Example PRIVATE tomlplusplus::tomlplusplus MREST asio)
target_include_directories(Example PUBLIC ${CMAKE_SOURCE_DIR}/include/mrest asio)