add_definitions(-DTRANSLATION_DOMAIN=\"kcm_users\")

add_subdirectory(src)

configure_file(config-user.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/user.h)

install( DIRECTORY avatars/ DESTINATION ${KDE_INSTALL_DATADIR}/plasma/avatars )
