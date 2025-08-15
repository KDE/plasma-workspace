set(libdbusmenu_SRCS
    dbusmenuimporter.cpp
    dbusmenuimporter.h
    dbusmenumodel.cpp
    dbusmenushortcut_p.cpp
    dbusmenushortcut_p.h
    dbusmenutypes_p.cpp
    dbusmenutypes_p.h
    dbusmenuview.cpp
    utils.cpp
)

ecm_qt_declare_logging_category(libdbusmenu_SRCS HEADER debug.h
                                            IDENTIFIER DBUSMENUQT
                                            CATEGORY_NAME org.kde.libdbusmenuqt
                                            DEFAULT_SEVERITY Info
                                            DESCRIPTION "libdbusmenu debug"
                                            EXPORT PLASMAWORKSPACE
                                        )

set_source_files_properties(com.canonical.dbusmenu.xml PROPERTIES
   NO_NAMESPACE true
   INCLUDE "dbusmenutypes_p.h"
   CLASSNAME DBusMenuInterface
)
qt_add_dbus_interface(libdbusmenu_SRCS com.canonical.dbusmenu.xml dbusmenu_interface)


add_library(dbusmenuqt SHARED ${libdbusmenu_SRCS})
target_link_libraries(dbusmenuqt
    Qt::DBus
    Qt::Widgets
)

target_include_directories(dbusmenuqt PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")

generate_export_header(dbusmenuqt)

install(TARGETS dbusmenuqt ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

add_subdirectory(test)
