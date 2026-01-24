# SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
# SPDX-License-Identifier: BSD-2-Clause

set(waydroidintegrationplugin_SRCS
    waydroidapplicationdbusobject.cpp
    waydroidapplicationdbusclient.cpp
    waydroidapplicationlistmodel.cpp
    waydroiddbusclient.cpp
    waydroiddbusobject.cpp
)

qt_generate_dbus_interface(
    ${CMAKE_CURRENT_SOURCE_DIR}/waydroiddbusobject.h
    org.kde.plasmashell.Waydroid.xml
    OPTIONS -s -m -P
)
qt_generate_dbus_interface(
    ${CMAKE_CURRENT_SOURCE_DIR}/waydroidapplicationdbusobject.h
    org.kde.plasmashell.WaydroidApplication.xml
    OPTIONS -s -m -P
)
qt_add_dbus_adaptor(waydroidintegrationplugin_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.Waydroid.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/waydroiddbusobject.h WaydroidDBusObject
)
qt_add_dbus_adaptor(waydroidintegrationplugin_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.WaydroidApplication.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/waydroidapplicationdbusobject.h WaydroidApplicationDBusObject
)
qt_add_dbus_interface(waydroidintegrationplugin_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.Waydroid.xml
    plasmashellwaydroidinterface
)
qt_add_dbus_interface(waydroidintegrationplugin_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.WaydroidApplication.xml
    plasmashellwaydroidapplicationinterface
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.Waydroid.xml
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.WaydroidApplication.xml
    DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR}
)

ecm_add_qml_module(waydroidintegrationplugin URI org.kde.plasma.private.workspace.waydroidintegrationplugin GENERATE_PLUGIN_SOURCE)
target_sources(waydroidintegrationplugin PRIVATE ${waydroidintegrationplugin_SRCS} ${RESOURCES})

target_link_libraries(waydroidintegrationplugin PRIVATE
    Qt::DBus
    Qt::Gui
    Qt::Qml
    Qt::Quick
    KF6::AuthCore
    KF6::ConfigCore
    KF6::I18n
    QCoro::Core
    QCoro::DBus
    QCoro::Qml
)

ecm_finalize_qml_module(waydroidintegrationplugin)

ecm_qt_declare_logging_category(waydroidintegrationplugin
    HEADER waydroidintegrationplugin_debug.h
    IDENTIFIER WAYDROIDINTEGRATIONPLUGIN
    DEFAULT_SEVERITY Warning
    CATEGORY_NAME org.kde.plasma.private.workspace.waydroidintegrationplugin
    DESCRIPTION "Plugin to add Waydroid Integration on Plasma"
)

add_subdirectory(kauth)

# Required for QCoro to compiles
kde_enable_exceptions()
