add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_recentdocuments\")

kcoreaddons_add_plugin(krunner_recentdocuments SOURCES recentdocuments.cpp INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_recentdocuments
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::I18n
    KF6::ActivitiesStats
    KF6::Runner
    KF6::Notifications
)
