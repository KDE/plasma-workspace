set(keyboardlayoutplugin_SRCS
    keyboardlayout.cpp
    keyboardlayoutplugin.cpp)

add_library(keyboardlayoutplugin SHARED ${keyboardlayoutplugin_SRCS})

target_link_libraries(keyboardlayoutplugin Qt5::Core
                                           Qt5::DBus
                                           Qt5::Qml)

set(keyboardlayoutplugin_PATH /org/kde/plasma/workspace/keyboardlayout)
install(TARGETS keyboardlayoutplugin
  DESTINATION ${KDE_INSTALL_QMLDIR}${keyboardlayoutplugin_PATH})
install(FILES qmldir
  DESTINATION ${KDE_INSTALL_QMLDIR}${keyboardlayoutplugin_PATH})
