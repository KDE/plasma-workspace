add_library(remotedirnotify MODULE remotedirnotify.cpp remotedirnotifymodule.cpp ../debug.cpp)
kcoreaddons_desktop_to_json(remotedirnotify remotedirnotify.desktop)

target_link_libraries(remotedirnotify KF5::DBusAddons KF5::KIOCore KF5::KDELibs4Support)

install(TARGETS remotedirnotify DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded )
