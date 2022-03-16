add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_placesrunner\")

set(krunner_placesrunner_SRCS
    placesrunner.cpp
    placesrunner.h
)

kcoreaddons_add_plugin(krunner_placesrunner SOURCES ${krunner_placesrunner_SRCS} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_placesrunner
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::I18n
    KF5::Runner
    KF5::Notifications
)
