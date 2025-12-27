ecm_add_qml_module(keyboardlayoutplugin URI "org.kde.plasma.workspace.keyboardlayout" VERSION 1.0 GENERATE_PLUGIN_SOURCE)

set_source_files_properties(org.kde.KeyboardLayouts.xml
        PROPERTIES INCLUDE layoutnames.h)

qt_add_dbus_interface(keyboardlayoutplugin_SRCS "org.kde.KeyboardLayouts.xml" keyboard_layout_interface)
qt_add_dbus_interface(keyboardlayoutplugin_SRCS "${KWIN_VIRTUALKEYBOARD_INTERFACE}" virtualkeyboard_interface)

target_sources(keyboardlayoutplugin PRIVATE
    keyboardlayout.cpp keyboardlayout.h
    layoutnames.cpp layoutnames.h
    virtualkeyboard.cpp virtualkeyboard.h
    ${keyboardlayoutplugin_SRCS}
)

ecm_qt_declare_logging_category(keyboardlayoutplugin HEADER debug.h
    IDENTIFIER KEYBOARD_LAYOUT
    CATEGORY_NAME kde.keyboardlayout
    DEFAULT_SEVERITY Info
    DESCRIPTION "keyboard layout plugins"
    EXPORT PLASMAWORKSPACE
)

target_link_libraries(keyboardlayoutplugin PRIVATE Qt::Core
                                           Qt::DBus
                                           Qt::Qml)

ecm_finalize_qml_module(keyboardlayoutplugin)
