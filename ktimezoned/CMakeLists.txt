if (WIN32)
    set(kded_ktimezoned_SRCS ktimezoned_win.cpp)
else ()
    set(kded_ktimezoned_SRCS ktimezoned.cpp)
endif ()

add_library(kded_ktimezoned MODULE ${kded_ktimezoned_SRCS})

kservice_desktop_to_json(kded_ktimezoned ktimezoned.desktop)

#qt5_add_dbus_adaptor(kded_ktimezoned_SRCS org.kde.KTimeZoned ktimezonedbus.h DBusHandler)

target_link_libraries(kded_ktimezoned
   Qt5::Core
   Qt5::DBus
   KF5::Service    # plugin factory
   KF5::CoreAddons # KDirWatch
   KF5::DBusAddons # kdedmodule
)

install(TARGETS kded_ktimezoned DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES ktimezoned.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded )


