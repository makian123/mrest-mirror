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

FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-36-0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(asio)

option(USE_EXAMPLE "Compile example subproject" ON)

add_subdirectory(src)

if(USE_EXAMPLE)
    add_subdirectory(example)
endif()