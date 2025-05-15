cmake_minimum_required(VERSION 3.15)
project(test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vulkan-renderer REQUIRED)
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

add_executable(${PROJECT_NAME} test.cpp)

target_include_directories(${PROJECT_NAME}
  PRIVATE 
    ${Vulkan_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
  PRIVATE
  ${Vulkan_LIBRARIES}
  ${SDL2_LIBRARIES}
  vulkan-renderer::vulkan-renderer
)

