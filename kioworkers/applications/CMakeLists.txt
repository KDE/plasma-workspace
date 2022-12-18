add_definitions(-DTRANSLATION_DOMAIN=\"kio5_applications\")

kcoreaddons_add_plugin(kio_applications SOURCES kio_applications.cpp INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kio")
set_target_properties(kio_applications PROPERTIES OUTPUT_NAME "applications")

target_link_libraries(kio_applications KF5::KIOCore KF5::I18n KF5::Service Qt::Network)
