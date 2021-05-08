if (WIN32)
    set(kded_ktimezoned_SRCS ktimezoned_win.cpp)
else ()
    set(kded_ktimezoned_SRCS ktimezoned.cpp)
endif ()

kcoreaddons_add_plugin(ktimezoned SOURCES ${kded_ktimezoned_SRCS} INSTALL_NAMESPACE "kf5/kded")
kcoreaddons_desktop_to_json(ktimezoned ktimezoned.desktop)

#qt_add_dbus_adaptor(kded_ktimezoned_SRCS org.kde.KTimeZoned ktimezonedbus.h DBusHandler)

target_link_libraries(ktimezoned
   Qt::Core
   Qt::DBus
   KF5::Service    # plugin factory
   KF5::CoreAddons # KDirWatch
   KF5::DBusAddons # kdedmodule
)
