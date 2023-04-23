add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_placesrunner\")

kcoreaddons_add_plugin(krunner_placesrunner SOURCES placesrunner.h placesrunner.cpp INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(krunner_placesrunner
    KF6::KIOCore
    KF6::KIOFileWidgets
    KF6::I18n
    KF6::Runner
    KF6::Notifications
)
