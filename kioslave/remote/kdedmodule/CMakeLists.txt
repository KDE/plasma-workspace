add_library(kded_remotedirnotify MODULE remotedirnotify.cpp remotedirnotifymodule.cpp ../kio_remote_debug.cpp)
target_link_libraries(kded_remotedirnotify KF5::DBusAddons KF5::KIOCore KF5::KDELibs4Support)

install(TARGETS kded_remotedirnotify  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES remotedirnotify.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )

