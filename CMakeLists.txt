cmake_minimum_required(VERSION 3.12)

project(vulkan-renderer
  VERSION 1.2.0
  DESCRIPTION "vulkan-renderer is a piping library designed as an alternative to for-loops."
  HOMEPAGE_URL "https://github.com/thom9258/vulkan-renderer/"
)


add_library(${PROJECT_NAME} SHARED)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}) 

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

find_package(polymorph)
find_package(glm)
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-terminate")

target_include_directories(${PROJECT_NAME} 
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanRenderer/
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/
  ${glm_INCLUDE_DIRS}
  ${glm_SOURCE_DIR}
  ${Vulkan_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIRS}
)
 
set(PUBLIC_HEADER_PATH /include/VulkanRenderer)

set(PUBLIC_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Context.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Bitmap.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Canvas.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/DebugMessenger.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Extent.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/GeometryPass.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/DescriptorPool.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/glm.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Mesh.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Renderable.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/ShaderTexture.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/StrongType.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Texture.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Utils.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/VertexBuffer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/VertexPosNormColor.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/VertexPosNormColorUV.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Presenter.hpp

  # TODO: THIRDPARTY TO BE HIDED AWAY
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/stb_image.h
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/tiny_obj_loader.hpp
)

set(PRIVATE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ContextImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/PresenterImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/GeometryPassImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/DescriptorPoolImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/VertexBufferImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ShaderTextureImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Canvas.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Bitmap.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Mesh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Texture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/VertexPosNormColor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/VertexPosNormColorUV.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/DebugMessenger.cpp
)

target_sources(${PROJECT_NAME} PRIVATE ${PRIVATE_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${PUBLIC_HEADERS})

set_property(TARGET ${PROJECT_NAME}
  PROPERTY PUBLIC_HEADER 
  ${PUBLIC_HEADERS}
)

target_link_libraries(${PROJECT_NAME} 
  polymorph::polymorph 
  glm::glm
  ${Vulkan_LIBRARIES}
  ${SDL2_LIBRARIES}
)

install (TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}${PUBLIC_HEADER_PATH}
)
