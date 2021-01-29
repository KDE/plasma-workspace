set(keyboardlayoutplugin_SRCS
    keyboardlayout.cpp
    keyboardlayoutplugin.cpp
    layoutnames.cpp
    layoutnames.h
    )

ecm_qt_declare_logging_category(keyboardlayoutplugin_SRCS HEADER debug.h
    IDENTIFIER KEYBOARD_LAYOUT
    CATEGORY_NAME kde.keyboardlayout
    DEFAULT_SEVERITY Info)

set_source_files_properties(org.kde.KeyboardLayouts.xml
        PROPERTIES INCLUDE layoutnames.h)

qt5_add_dbus_interface(keyboardlayoutplugin_SRCS "org.kde.KeyboardLayouts.xml" keyboard_layout_interface)

add_library(keyboardlayoutplugin SHARED ${keyboardlayoutplugin_SRCS})

target_link_libraries(keyboardlayoutplugin Qt::Core
                                           Qt::DBus
                                           Qt::Qml)

set(keyboardlayoutplugin_PATH /org/kde/plasma/workspace/keyboardlayout)
install(TARGETS keyboardlayoutplugin
  DESTINATION ${KDE_INSTALL_QMLDIR}${keyboardlayoutplugin_PATH})
install(FILES qmldir
  DESTINATION ${KDE_INSTALL_QMLDIR}${keyboardlayoutplugin_PATH})
