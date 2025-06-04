cmake_minimum_required(VERSION 3.12)

project(vulkan-renderer
  VERSION 0.1.0
  DESCRIPTION "vulkan-renderer is a piping library designed as an alternative to for-loops."
  HOMEPAGE_URL "https://github.com/thom9258/vulkan-renderer/"
)

include(GNUInstallDirs)

add_library(${PROJECT_NAME} INTERFACE)
# allows other projects add_subdirectory() this project
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}) 



set(vulkan-renderer_INCLUDE_DIR "VulkanRenderer")

target_include_directories(${PROJECT_NAME} 
  INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}_Targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("vulkan-rendererConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/vulkan-rendererConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/vulkan-rendererConfig.cmake"
  INSTALL_DESTINATION
  "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake"
)

install(
  EXPORT ${PROJECT_NAME}_Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake"
)

install(
  FILES
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  DESTINATION
  "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake"
)

install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/${vulkan-renderer_INCLUDE_DIR}
  DESTINATION include
)
