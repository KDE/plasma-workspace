add_definitions(-DTRANSLATION_DOMAIN=\"kio_applications\")

kcoreaddons_add_plugin(kio_applications SOURCES kio_applications.cpp INSTALL_NAMESPACE "kf6/kio")
set_target_properties(kio_applications PROPERTIES OUTPUT_NAME "applications")

target_link_libraries(kio_applications KF6::KIOCore KF6::I18n KF6::Service Qt::Network)
