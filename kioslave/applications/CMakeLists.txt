add_library(kio_applications MODULE kio_applications.cpp)

target_link_libraries(kio_applications KF5::KDE4Support KF5::KIOCore)

install(TARGETS kio_applications  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES  programs.protocol applications.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
