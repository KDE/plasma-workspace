add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")

set(kded_statusnotifierwatcher_SRCS statusnotifierwatcher.cpp )

qt_add_dbus_adaptor(kded_statusnotifierwatcher_SRCS     ${KNOTIFICATIONS_DBUS_INTERFACES_DIR}/kf5_org.kde.StatusNotifierWatcher.xml
                     statusnotifierwatcher.h StatusNotifierWatcher)


set(statusnotifieritem_xml ${KNOTIFICATIONS_DBUS_INTERFACES_DIR}/kf5_org.kde.StatusNotifierItem.xml)
set_source_files_properties(${statusnotifieritem_xml} PROPERTIES
   NO_NAMESPACE false
   INCLUDE "systemtraytypedefs.h"
   CLASSNAME OrgKdeStatusNotifierItemInterface
)
qt_add_dbus_interface(kded_statusnotifierwatcher_SRCS ${statusnotifieritem_xml} statusnotifieritem_interface)

kcoreaddons_add_plugin(statusnotifierwatcher SOURCES ${kded_statusnotifierwatcher_SRCS} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kded")
target_link_libraries(statusnotifierwatcher Qt::DBus KF5::DBusAddons KF5::CoreAddons)
