cmake_minimum_required (VERSION 3.19)

project("MREST")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -freflection")
set(glaze_ENABLE_REFLECTION26 ON)

include(FetchContent)
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v7.5.0
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glaze)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_SOURCE_DIR}/deps/asio-1.36.0/include)
target_link_libraries(asio INTERFACE pthread)

option(USE_EXAMPLE "Compile example subproject" ON)

add_subdirectory(src)

if(USE_EXAMPLE)
    add_subdirectory(example)
endif()