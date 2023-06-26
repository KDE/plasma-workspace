kcoreaddons_add_plugin(ktimezoned SOURCES ktimezoned.cpp ktimezoned.h INSTALL_NAMESPACE "kf6/kded")

#qt_add_dbus_adaptor(kded_ktimezoned_SRCS org.kde.KTimeZoned ktimezonedbus.h DBusHandler)

target_link_libraries(ktimezoned
   Qt::Core
   Qt::DBus
   KF6::Service    # plugin factory
   KF6::CoreAddons # KDirWatch
   KF6::DBusAddons # kdedmodule
   KF6::ConfigCore # KConfig[Group]
)
