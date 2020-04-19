add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

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

add_library(statusnotifierwatcher MODULE ${kded_statusnotifierwatcher_SRCS})
kcoreaddons_desktop_to_json(statusnotifierwatcher statusnotifierwatcher.desktop)


target_link_libraries(statusnotifierwatcher Qt5::DBus KF5::DBusAddons KF5::CoreAddons)

install(TARGETS statusnotifierwatcher  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded)
