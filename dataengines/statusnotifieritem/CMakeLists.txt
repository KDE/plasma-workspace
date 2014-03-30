project(plasma-statusnotifieritem_engine)

include_directories(${plasma-workspace_SOURCE_DIR}/statusnotifierwatcher)
include_directories(${dbusmenu-qt5_INCLUDE_DIRS})


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

set(statusnotifieritem_xml ${KNOTIFICATIONS_DBUS_INTERFACES_DIR}/kf5_org.kde.StatusNotifierItem.xml)

set_source_files_properties(${statusnotifieritem_xml} PROPERTIES
   NO_NAMESPACE false
   INCLUDE "systemtraytypes.h"
   CLASSNAME OrgKdeStatusNotifierItem
)
qt5_add_dbus_interface(statusnotifieritem_engine_SRCS ${statusnotifieritem_xml} statusnotifieritem_interface)

add_library(plasma_engine_statusnotifieritem MODULE ${statusnotifieritem_engine_SRCS})
target_link_libraries(plasma_engine_statusnotifieritem
    Qt5::DBus
    KF5::Service
    KF5::Plasma
    KF5::IconThemes
    KF5::KDE4Support
    dbusmenu-qt5
)

kservice_desktop_to_json(plasma_engine_statusnotifieritem plasma-engine-statusnotifieritem.desktop)

install(TARGETS plasma_engine_statusnotifieritem DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-engine-statusnotifieritem.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES statusnotifieritem.operations DESTINATION ${DATA_INSTALL_DIR}/plasma/services)

