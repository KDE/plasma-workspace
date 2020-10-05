ecm_install_configured_files(INPUT plasma-ksplash-ready.service.in @ONLY
                                                   DESTINATION ${SYSTEMD_USER_UNIT_INSTALL_DIR})

install(FILES plasma-core.target DESTINATION ${SYSTEMD_USER_UNIT_INSTALL_DIR})
install(FILES plasma-workspace@.target DESTINATION ${SYSTEMD_USER_UNIT_INSTALL_DIR})

add_executable(kde-systemd-start-condition kde-systemd-start-condition.cpp)
target_link_libraries(kde-systemd-start-condition PUBLIC KF5::ConfigCore KF5::Service)
install(TARGETS kde-systemd-start-condition DESTINATION ${KDE_INSTALL_BINDIR})
