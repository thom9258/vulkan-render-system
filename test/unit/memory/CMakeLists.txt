cmake_minimum_required(VERSION 3.1)
project(test)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -ggdb")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} main.cpp)

include(FetchContent)
message(STATUS "Fetching Libtester")
FetchContent_Declare(libtester
  GIT_REPOSITORY git@github.com:thom9258/libtester.git
  GIT_TAG main
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(libtester)

target_sources(test PRIVATE ../../../source/Arena.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE libtester)
