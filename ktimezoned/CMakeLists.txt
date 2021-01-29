if (WIN32)
    set(kded_ktimezoned_SRCS ktimezoned_win.cpp)
else ()
    set(kded_ktimezoned_SRCS ktimezoned.cpp)
endif ()

add_library(ktimezoned MODULE ${kded_ktimezoned_SRCS})
kcoreaddons_desktop_to_json(ktimezoned ktimezoned.desktop)

#qt5_add_dbus_adaptor(kded_ktimezoned_SRCS org.kde.KTimeZoned ktimezonedbus.h DBusHandler)

target_link_libraries(ktimezoned
   Qt::Core
   Qt::DBus
   KF5::Service    # plugin factory
   KF5::CoreAddons # KDirWatch
   KF5::DBusAddons # kdedmodule
)

install(TARGETS ktimezoned DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded)
