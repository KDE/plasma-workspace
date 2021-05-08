kcoreaddons_add_plugin(plasma_applet_appmenu SOURCES appmenuapplet.cpp INSTALL_NAMESPACE "plasma/applets")
kcoreaddons_desktop_to_json(plasma_applet_appmenu ../package/metadata.desktop)

target_link_libraries(plasma_applet_appmenu
                      Qt::Widgets
                      Qt::Quick
                      Qt::DBus
                      KF5::Plasma
                      KF5::WindowSystem
                      PW::LibTaskManager)
