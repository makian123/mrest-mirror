cmake_minimum_required (VERSION 3.19)

project("MREST")

set(CMAKE_CXX_STANDARD 26)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -freflection")

include(FetchContent)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)
FetchContent_Declare(
    nlohmann
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_SOURCE_DIR}/deps/asio-1.36.0/include)
target_link_libraries(asio INTERFACE pthread)

add_subdirectory(src)
add_subdirectory(example)