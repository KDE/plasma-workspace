add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_shell\")

kcoreaddons_add_plugin(krunner_shell SOURCES shellrunner.cpp INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_shell
    KF5::KIOCore
    KF5::KIOGui
    KF5::I18n
    KF5::Notifications
    KF5::Plasma
    KF5::Runner
    KF5::Completion
)

if(BUILD_TESTING)
add_subdirectory(autotests)
endif()

