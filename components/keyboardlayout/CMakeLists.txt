set(keyboardlayoutplugin_SRCS
    keyboardlayout.cpp
    keyboardlayoutplugin.cpp)


ecm_qt_declare_logging_category(keyboardlayoutplugin_SRCS HEADER debug.h
    IDENTIFIER KEYBOARD_LAYOUT
    CATEGORY_NAME kde.keyboardlayout
    DEFAULT_SEVERITY Info)

add_library(keyboardlayoutplugin SHARED ${keyboardlayoutplugin_SRCS})

target_link_libraries(keyboardlayoutplugin Qt5::Core
                                           Qt5::DBus
                                           Qt5::Qml)

set(keyboardlayoutplugin_PATH /org/kde/plasma/workspace/keyboardlayout)
install(TARGETS keyboardlayoutplugin
  DESTINATION ${KDE_INSTALL_QMLDIR}${keyboardlayoutplugin_PATH})
install(FILES qmldir
  DESTINATION ${KDE_INSTALL_QMLDIR}${keyboardlayoutplugin_PATH})
