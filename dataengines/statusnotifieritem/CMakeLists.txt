include_directories(${plasma-workspace_SOURCE_DIR}/statusnotifierwatcher)

# We add our source code here
set(statusnotifieritem_engine_SRCS
    statusnotifieritem_engine.cpp
    statusnotifieritemsource.cpp
    statusnotifieritemservice.cpp
    statusnotifieritemjob.cpp
    systemtraytypes.cpp
    statusnotifieritem_engine.h
    statusnotifieritemsource.h
    statusnotifieritemservice.h
    statusnotifieritemjob.h
    systemtraytypes.h
)

set(statusnotifierwatcher_xml ${KNOTIFICATIONS_DBUS_INTERFACES_DIR}/kf5_org.kde.StatusNotifierWatcher.xml)
qt_add_dbus_interface(statusnotifieritem_engine_SRCS ${statusnotifierwatcher_xml} statusnotifierwatcher_interface)
qt_add_dbus_interface(statusnotifieritem_engine_SRCS ../mpris2/org.freedesktop.DBus.Properties.xml dbusproperties)

set(statusnotifieritem_xml ${KNOTIFICATIONS_DBUS_INTERFACES_DIR}/kf5_org.kde.StatusNotifierItem.xml)

set_source_files_properties(${statusnotifieritem_xml} PROPERTIES
   NO_NAMESPACE false
   INCLUDE "systemtraytypes.h"
   CLASSNAME OrgKdeStatusNotifierItem
)
qt_add_dbus_interface(statusnotifieritem_engine_SRCS ${statusnotifieritem_xml} statusnotifieritem_interface)

ecm_qt_declare_logging_category(statusnotifieritem_engine_SRCS HEADER debug.h
                                               IDENTIFIER DATAENGINE_SNI
                                               CATEGORY_NAME kde.dataengine.sni
                                               DEFAULT_SEVERITY Info)

kcoreaddons_add_plugin(plasma_engine_statusnotifieritem SOURCES ${statusnotifieritem_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)
target_link_libraries(plasma_engine_statusnotifieritem
    Qt::DBus
    KF6::Service
    KF6::Plasma
    KF6::Plasma5Support
    KF6::IconThemes
    KF6::WindowSystem
    dbusmenuqt
)

install(FILES statusnotifieritem.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)

