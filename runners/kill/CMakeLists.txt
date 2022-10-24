add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_kill\")

set(kcm_krunner_kill_SRCS
    killrunner_config.cpp
    killrunner_config.h
)

ki18n_wrap_ui(kcm_krunner_kill_SRCS killrunner_config.ui)
kcoreaddons_add_plugin(kcm_krunner_kill SOURCES ${kcm_krunner_kill_SRCS} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner/kcms")
target_compile_definitions(kcm_krunner_kill PRIVATE -DKRUNNER_PLUGIN_NAME="krunner_kill")
target_link_libraries(kcm_krunner_kill
    Qt::Core
    Qt::Gui
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::I18n
    KF5::Completion
    KF5::ConfigWidgets
    KF5::Runner
    )

kcoreaddons_add_plugin(krunner_kill SOURCES killrunner.cpp INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_kill
                      KF5::I18n
                      KF5::Completion
                      KF5::ConfigWidgets
                      KF5::AuthCore
                      KF5::Runner
                      KSysGuard::ProcessCore
                      )
