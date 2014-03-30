project(StatusNotifierWatcher)

set(kded_statusnotifierwatcher_SRCS statusnotifierwatcher.cpp )

qt5_add_dbus_adaptor(kded_statusnotifierwatcher_SRCS     ${KNOTIFICATIONS_DBUS_INTERFACES_DIR}/kf5_org.kde.StatusNotifierWatcher.xml
                     statusnotifierwatcher.h StatusNotifierWatcher)


set(statusnotifieritem_xml ${KNOTIFICATIONS_DBUS_INTERFACES_DIR}/kf5_org.kde.StatusNotifierItem.xml)
set_source_files_properties(${statusnotifieritem_xml} PROPERTIES
   NO_NAMESPACE false
   INCLUDE "systemtraytypedefs.h"
   CLASSNAME OrgKdeStatusNotifierItemInterface
)
qt5_add_dbus_interface(kded_statusnotifierwatcher_SRCS ${statusnotifieritem_xml} statusnotifieritem_interface)

add_library(kded_statusnotifierwatcher MODULE ${kded_statusnotifierwatcher_SRCS})


target_link_libraries(kded_statusnotifierwatcher Qt5::DBus KF5::DBusAddons KF5::KDE4Support )

install(TARGETS kded_statusnotifierwatcher  DESTINATION ${PLUGIN_INSTALL_DIR})


install( FILES statusnotifierwatcher.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/kded)

