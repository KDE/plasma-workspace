set(appmenuapplet_SRCS
    appmenuapplet.cpp
)

add_library(plasma_applet_appmenu MODULE ${appmenuapplet_SRCS})

kcoreaddons_desktop_to_json(plasma_applet_appmenu ../package/metadata.desktop)

target_link_libraries(plasma_applet_appmenu
                      Qt::Widgets
                      Qt::Quick
                      Qt::DBus
                      KF5::Plasma
                      KF5::WindowSystem
                      PW::LibTaskManager)

install(TARGETS plasma_applet_appmenu DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)
