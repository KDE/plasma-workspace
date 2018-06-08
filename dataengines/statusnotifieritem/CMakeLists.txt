include_directories(${plasma-workspace_SOURCE_DIR}/statusnotifierwatcher)

# We add our source code here
set(statusnotifieritem_engine_SRCS
    statusnotifieritem_engine.cpp
    statusnotifieritemsource.cpp
    statusnotifieritemservice.cpp
    statusnotifieritemjob.cpp
    systemtraytypes.cpp
)

set(statusnotifierwatcher_xml ${KNOTIFICATIONS_DBUS_INTERFACES_DIR}/kf5_org.kde.StatusNotifierWatcher.xml)
qt5_add_dbus_interface(statusnotifieritem_engine_SRCS ${statusnotifierwatcher_xml} statusnotifierwatcher_interface)
qt5_add_dbus_interface(statusnotifieritem_engine_SRCS ../mpris2/org.freedesktop.DBus.Properties.xml dbusproperties)

set(statusnotifieritem_xml ${KNOTIFICATIONS_DBUS_INTERFACES_DIR}/kf5_org.kde.StatusNotifierItem.xml)

set_source_files_properties(${statusnotifieritem_xml} PROPERTIES
   NO_NAMESPACE false
   INCLUDE "systemtraytypes.h"
   CLASSNAME OrgKdeStatusNotifierItem
)
qt5_add_dbus_interface(statusnotifieritem_engine_SRCS ${statusnotifieritem_xml} statusnotifieritem_interface)

ecm_qt_declare_logging_category(statusnotifieritem_engine_SRCS HEADER debug.h
                                               IDENTIFIER DATAENGINE_SNI
                                               CATEGORY_NAME kde.dataengine.sni
                                               DEFAULT_SEVERITY Info)

add_library(plasma_engine_statusnotifieritem MODULE ${statusnotifieritem_engine_SRCS})
target_link_libraries(plasma_engine_statusnotifieritem
    Qt5::DBus
    KF5::Service
    KF5::Plasma
    KF5::IconThemes
    dbusmenuqt
)

kcoreaddons_desktop_to_json(plasma_engine_statusnotifieritem plasma-dataengine-statusnotifieritem.desktop)

install(TARGETS plasma_engine_statusnotifieritem DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-statusnotifieritem.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES statusnotifieritem.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)

