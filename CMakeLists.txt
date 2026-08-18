cmake_minimum_required(VERSION 3.21)

project(RLyehPnakotic VERSION 0.0.0 LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(RLYEH_PNAKOTIC_BUILD_TESTS "Build R'Lyeh Pnakotic tests" ${PROJECT_IS_TOP_LEVEL})
option(RLYEH_PNAKOTIC_BUILD_APP "Build the R'Lyeh Pnakotic Qt application" OFF)

add_library(RLyehPnakotic INTERFACE)
add_library(RLyeh::Pnakotic ALIAS RLyehPnakotic)
set_target_properties(RLyehPnakotic PROPERTIES EXPORT_NAME Pnakotic)

target_compile_features(RLyehPnakotic INTERFACE cxx_std_20)
target_include_directories(RLyehPnakotic INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_sources(RLyehPnakotic INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/RLyehPnakotic/Codec.h>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/RLyehPnakotic/Pnakotic.h>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/RLyehPnakotic/ProjectPolicy.h>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/RLyehPnakotic/Version.h>
)

set(RLyehPnakotic_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/RLyehPnakotic"
    CACHE STRING "Install location for the R'Lyeh Pnakotic CMake package")

configure_package_config_file(
  cmake/RLyehPnakoticConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/RLyehPnakoticConfig.cmake"
  INSTALL_DESTINATION "${RLyehPnakotic_INSTALL_CMAKEDIR}"
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/RLyehPnakoticConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMinorVersion
)

install(TARGETS RLyehPnakotic
  EXPORT RLyehPnakoticTargets
  COMPONENT RLyehPnakotic
)
install(DIRECTORY include/RLyehPnakotic
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT RLyehPnakotic
)
install(EXPORT RLyehPnakoticTargets
  FILE RLyehPnakoticTargets.cmake
  NAMESPACE RLyeh::
  DESTINATION "${RLyehPnakotic_INSTALL_CMAKEDIR}"
  COMPONENT RLyehPnakotic
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/RLyehPnakoticConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/RLyehPnakoticConfigVersion.cmake"
  DESTINATION "${RLyehPnakotic_INSTALL_CMAKEDIR}"
  COMPONENT RLyehPnakotic
)
install(FILES LICENSE
  DESTINATION "${CMAKE_INSTALL_DATADIR}/RLyehPnakotic"
  COMPONENT RLyehPnakotic
)

if(RLYEH_PNAKOTIC_BUILD_APP)
  find_package(Qt6 6.11 REQUIRED COMPONENTS Quick QuickControls2)
  qt_standard_project_setup(REQUIRES 6.8)
  add_subdirectory(app)
endif()

if(RLYEH_PNAKOTIC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
