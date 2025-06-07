cmake_minimum_required(VERSION 3.2)

project(renderer VERSION 0.1.0)

include(FetchContent)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -ggdb")
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)
find_package(vulkan-renderer CONFIG REQUIRED)

#-------------------------------------------------------------------------
# Fetch SimpleGeometry
# ------------------------------------------------------------------------
message(STATUS "Fetching Simple Geometry")
FetchContent_Declare(SimpleGeometry
    GIT_REPOSITORY git@github.com:thom9258/SimpleGeometry.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(SimpleGeometry)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME}
  PRIVATE 
    ${Vulkan_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
  PRIVATE
  SimpleGeometry
  ${Vulkan_LIBRARIES}
  ${SDL2_LIBRARIES}
  vulkan-renderer::vulkan-renderer
)
