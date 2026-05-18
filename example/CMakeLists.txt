cmake_minimum_required (VERSION 3.19)

project("Example")

file(GLOB_RECURSE SOURCES *.cpp)

add_executable(Example ${SOURCES})
target_link_libraries(Example PRIVATE MREST glaze::glaze)
target_include_directories(Example PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/mrest ${asio_SOURCE_DIR}/asio/include)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/configuration.toml
    ${CMAKE_CURRENT_BINARY_DIR}/configuration.toml
    COPYONLY)