add_definitions(-DTRANSLATION_DOMAIN=\"kcm_users\")

add_subdirectory(src)

install( DIRECTORY avatars/ DESTINATION ${KDE_INSTALL_DATADIR}/plasma/avatars )
