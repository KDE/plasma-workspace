# SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

set_source_files_properties(com.canonical.dbusmenu.xml PROPERTIES
   NO_NAMESPACE true
   INCLUDE "dbusmenutypes_p.h"
   CLASSNAME DBusMenuInterface
)
qt_add_dbus_interface(libdbusmenu_GENERATED com.canonical.dbusmenu.xml dbusmenu_interface)

add_library(dbusmenuqt SHARED
    dbusmenuimporter.cpp
    dbusmenumodel.cpp
    dbusmenushortcut_p.cpp
    dbusmenutypes_p.cpp
    dbusmenuview.cpp
    utils.cpp

    ${libdbusmenu_GENERATED}
)

generate_export_header(dbusmenuqt)

ecm_qt_declare_logging_category(dbusmenuqt
    HEADER debug.h
    IDENTIFIER DBUSMENUQT
    CATEGORY_NAME org.kde.libdbusmenuqt
    DEFAULT_SEVERITY Info
    DESCRIPTION "libdbusmenu debug"
    EXPORT PLASMAWORKSPACE
)

target_link_libraries(dbusmenuqt PUBLIC
    Qt::DBus
    Qt::Widgets
)

target_include_directories(dbusmenuqt PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")

install(TARGETS dbusmenuqt ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

add_subdirectory(test)
