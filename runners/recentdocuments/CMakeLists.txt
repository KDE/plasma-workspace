add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_recentdocuments\")

kcoreaddons_add_plugin(krunner_recentdocuments SOURCES recentdocuments.cpp INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_recentdocuments
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    KF5::ActivitiesStats
    KF5::Runner
    KF5::Notifications
)
