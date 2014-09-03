set(keyboardlayoutplugin_SRCS
    keyboardlayout.cpp
    keyboardlayoutplugin.cpp)

add_library(keyboardlayoutplugin SHARED ${keyboardlayoutplugin_SRCS})

target_link_libraries(keyboardlayoutplugin Qt5::Core
                                           Qt5::DBus
                                           Qt5::Qml)

set(keyboardlayoutplugin_PATH /org/kde/plasma/workspace/keyboardlayout)
install(TARGETS keyboardlayoutplugin
  DESTINATION ${QML_INSTALL_DIR}${keyboardlayoutplugin_PATH})
install(FILES qmldir
  DESTINATION ${QML_INSTALL_DIR}${keyboardlayoutplugin_PATH})
