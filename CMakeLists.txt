cmake_minimum_required(VERSION 3.21)

project(RLyehPnakotic LANGUAGES CXX)

option(RLYEH_PNAKOTIC_BUILD_TESTS "Build R'Lyeh Pnakotic tests" ${PROJECT_IS_TOP_LEVEL})
option(RLYEH_PNAKOTIC_BUILD_APP "Build the R'Lyeh Pnakotic Qt application" OFF)

if(PROJECT_IS_TOP_LEVEL OR RLYEH_PNAKOTIC_BUILD_APP OR RLYEH_PNAKOTIC_BUILD_TESTS)
  include(cmake/PnakoticSCM.cmake)
endif()

include(cmake/RLyehPnakoticLibrary.cmake)

if(PROJECT_IS_TOP_LEVEL)
  add_executable(RLyehPnakoticPackageVersion EXCLUDE_FROM_ALL packaging/library/print_version.cpp)
  target_link_libraries(RLyehPnakoticPackageVersion PRIVATE RLyeh::Pnakotic)
  target_include_directories(RLyehPnakoticPackageVersion PRIVATE "${PNAKOTIC_GENERATED_DIR}")
  set_target_properties(RLyehPnakoticPackageVersion PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
  )
endif()

if(RLYEH_PNAKOTIC_BUILD_APP)
  find_package(Qt6 6.11 REQUIRED COMPONENTS Quick QuickControls2)
  qt_standard_project_setup(REQUIRES 6.8)
  add_subdirectory(app)
endif()

if(RLYEH_PNAKOTIC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
