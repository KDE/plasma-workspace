kcoreaddons_add_plugin(org.kde.plasma.appmenu SOURCES appmenuapplet.cpp INSTALL_NAMESPACE "plasma/applets")

target_link_libraries(org.kde.plasma.appmenu
                      Qt::Widgets
                      Qt::Quick
                      Qt::DBus
                      KF6::Plasma
                      KF6::WindowSystem
                      PW::LibTaskManager)
