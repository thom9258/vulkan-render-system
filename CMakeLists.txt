cmake_minimum_required(VERSION 3.12)

project(vulkan-renderer
  VERSION 1.5.1
  DESCRIPTION "vulkan renderer"
  HOMEPAGE_URL "https://github.com/thom9258/vulkan-renderer/"
)

add_library(${PROJECT_NAME} SHARED)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}) 

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(polymorph)
find_package(glm)
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)
find_package(assimp REQUIRED)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # gcc warning flags
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -g -ggdb -Werror=format-security -Werror=array-bounds)
endif()

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
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Exception.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Camera.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Context.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Bitmap.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Canvas.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/DebugMessenger.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Extent.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Utils.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Transform.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Renderer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/DescriptorPool.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/glm.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/FlightFrames.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Mesh.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/ModelLoader.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Animation.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Animator.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Renderable.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Light.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/ShadowCaster.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/ShaderTexture.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/TextureSamplerCache.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/MeshCache.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/StrongType.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Texture.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/Vertex.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/VertexBuffer.hpp

  # TODO: THIRDPARTY TO BE HIDED AWAY
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/stb_image.h
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/tiny_obj_loader.hpp

  # TODO: MOVE TO A NAMESPACE
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/external/Timer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/external/FPSCounter.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/external/EventSystem.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}${PUBLIC_HEADER_PATH}/external/SceneSystem.hpp
)

set(PRIVATE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Arena.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Renderable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Camera.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/FlightFrames.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ContextImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Presenter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ShadowPass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ShadowCaster.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/RendererImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/DescriptorPoolImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/VertexBufferImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ShaderTextureImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/AnimationUtils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/TextureSamplerCache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/MeshCache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/PipelineUtils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/MaterialPipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/AnimatedPipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/StaticDepthPipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/AnimatedDepthPipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Canvas.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Bitmap.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Mesh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ModelLoader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Animation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Animator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/TextureImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/VertexImpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/DebugMessenger.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/LightUniforms.cpp
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
  assimp::assimp
  ${Vulkan_LIBRARIES}
  ${SDL2_LIBRARIES}
)

install (TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}${PUBLIC_HEADER_PATH}
)
