add_definitions(-DTRANSLATION_DOMAIN=\"kio5_applications\")

add_library(kio_applications MODULE kio_applications.cpp)
set_target_properties(kio_applications PROPERTIES OUTPUT_NAME "applications")

target_link_libraries(kio_applications KF5::KIOCore KF5::I18n Qt5::Network)

install(TARGETS kio_applications  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
install( FILES  programs.protocol applications.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
