add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_placesrunner\")

set(krunner_placesrunner_SRCS
    placesrunner.cpp
    placesrunner.h
)

kcoreaddons_add_plugin(krunner_placesrunner SOURCES ${krunner_placesrunner_SRCS} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_placesrunner
    KF6::KIOCore
    KF6::KIOFileWidgets
    KF6::I18n
    KF6::Runner
    KF6::Notifications
)
