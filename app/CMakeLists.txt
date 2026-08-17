set(RLyehPnakotic_APP_ICON "${PROJECT_SOURCE_DIR}/assets/img/cryptex.icns")
set(RLyehPnakotic_APP_ARTWORK "${PROJECT_SOURCE_DIR}/assets/img/vaulketh-cryptex.png")
set(RLyehPnakotic_APP_FONT "${PROJECT_SOURCE_DIR}/assets/font/Metamorphous-Regular.ttf")
set(RLyehPnakotic_ASSET_LICENSE "${PROJECT_SOURCE_DIR}/assets/LICENSE")
set(RLyehPnakotic_FONT_LICENSE "${PROJECT_SOURCE_DIR}/assets/font/LICENSE/OFL.txt")

set_source_files_properties("${RLyehPnakotic_APP_ICON}" PROPERTIES
  MACOSX_PACKAGE_LOCATION Resources
)
set_source_files_properties("${RLyehPnakotic_APP_ARTWORK}" PROPERTIES
  QT_RESOURCE_ALIAS images/vaulketh-cryptex.png
)
set_source_files_properties("${RLyehPnakotic_APP_FONT}" PROPERTIES
  QT_RESOURCE_ALIAS fonts/Metamorphous-Regular.ttf
)
set_source_files_properties("${RLyehPnakotic_ASSET_LICENSE}" PROPERTIES
  QT_RESOURCE_ALIAS licenses/ASSETS.txt
)
set_source_files_properties("${RLyehPnakotic_FONT_LICENSE}" PROPERTIES
  QT_RESOURCE_ALIAS licenses/Metamorphous-OFL.txt
)
set_source_files_properties(qml/Main.qml PROPERTIES
  QT_RESOURCE_ALIAS Main.qml
)

qt_add_executable(RLyehPnakoticApp
  main.cpp
  PnakoticController.cpp
  PnakoticController.h
  "${RLyehPnakotic_APP_ICON}"
)

qt_add_qml_module(RLyehPnakoticApp
  URI RLyeh.Pnakotic
  VERSION 1.0
  QML_FILES
    qml/Main.qml
)

qt_add_resources(RLyehPnakoticApp app_assets
  PREFIX "/"
  FILES
    "${RLyehPnakotic_APP_ARTWORK}"
    "${RLyehPnakotic_APP_FONT}"
    "${RLyehPnakotic_ASSET_LICENSE}"
    "${RLyehPnakotic_FONT_LICENSE}"
)

target_link_libraries(RLyehPnakoticApp PRIVATE
  RLyeh::Pnakotic
  Qt6::Quick
  Qt6::QuickControls2
)

set_target_properties(RLyehPnakoticApp PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  MACOSX_BUNDLE TRUE
  MACOSX_BUNDLE_BUNDLE_NAME "R'Lyeh Pnakotic"
  MACOSX_BUNDLE_GUI_IDENTIFIER dev.rlyeh.pnakotic
  MACOSX_BUNDLE_ICON_FILE cryptex.icns
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
  MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
  WIN32_EXECUTABLE TRUE
)

install(TARGETS RLyehPnakoticApp
  BUNDLE DESTINATION .
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  COMPONENT RLyehPnakoticApp
)
