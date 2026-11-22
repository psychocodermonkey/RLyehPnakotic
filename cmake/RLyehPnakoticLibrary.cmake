include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

get_filename_component(PNAKOTIC_LIBRARY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

add_library(RLyehPnakotic INTERFACE)
add_library(RLyeh::Pnakotic ALIAS RLyehPnakotic)
set_target_properties(RLyehPnakotic PROPERTIES EXPORT_NAME Pnakotic)

target_compile_features(RLyehPnakotic INTERFACE cxx_std_20)
target_include_directories(RLyehPnakotic INTERFACE
  $<BUILD_INTERFACE:${PNAKOTIC_LIBRARY_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_sources(RLyehPnakotic INTERFACE
  $<BUILD_INTERFACE:${PNAKOTIC_LIBRARY_SOURCE_DIR}/include/RLyehPnakotic/Codec.h>
  $<BUILD_INTERFACE:${PNAKOTIC_LIBRARY_SOURCE_DIR}/include/RLyehPnakotic/Pnakotic.h>
  $<BUILD_INTERFACE:${PNAKOTIC_LIBRARY_SOURCE_DIR}/include/RLyehPnakotic/ProjectPolicy.h>
  $<BUILD_INTERFACE:${PNAKOTIC_LIBRARY_SOURCE_DIR}/include/RLyehPnakotic/Version.h>
)

set(RLyehPnakotic_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/RLyehPnakotic"
    CACHE STRING "Install location for the R'Lyeh Pnakotic CMake package")

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/RLyehPnakoticConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/RLyehPnakoticConfig.cmake"
  INSTALL_DESTINATION "${RLyehPnakotic_INSTALL_CMAKEDIR}"
)

install(TARGETS RLyehPnakotic
  EXPORT RLyehPnakoticTargets
  COMPONENT RLyehPnakotic
)
install(DIRECTORY "${PNAKOTIC_LIBRARY_SOURCE_DIR}/include/RLyehPnakotic"
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
  DESTINATION "${RLyehPnakotic_INSTALL_CMAKEDIR}"
  COMPONENT RLyehPnakotic
)
install(FILES "${PNAKOTIC_LIBRARY_SOURCE_DIR}/LICENSE"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/RLyehPnakotic"
  COMPONENT RLyehPnakotic
)
