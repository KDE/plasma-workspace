add_subdirectory(tests)

add_library(kio_desktop MODULE kio_desktop.cpp)

target_link_libraries(kio_desktop KF5::KIOCore KF5::KDELibs4Support)

set_target_properties(kio_desktop PROPERTIES OUTPUT_NAME "desktop")

install(TARGETS kio_desktop DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)

########### next target ###############

add_library(kded_desktopnotifier MODULE desktopnotifier.cpp)

target_link_libraries(kded_desktopnotifier KF5::KIOCore KF5::DBusAddons KF5::KDELibs4Support)

install(TARGETS kded_desktopnotifier  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES desktopnotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
install( FILES desktop.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

install(FILES directory.desktop directory.trash DESTINATION ${DATA_INSTALL_DIR}/kio_desktop)
install(FILES Home.desktop DESTINATION ${DATA_INSTALL_DIR}/kio_desktop/DesktopLinks)

