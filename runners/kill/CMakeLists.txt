add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_kill\")

set(kcm_krunner_kill_SRCS
    killrunner_config.cpp
)

ki18n_wrap_ui(kcm_krunner_kill_SRCS killrunner_config.ui)
add_library(kcm_krunner_kill MODULE ${kcm_krunner_kill_SRCS})
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

add_library(krunner_kill MODULE killrunner.cpp)
kcoreaddons_desktop_to_json(krunner_kill plasma-runner-kill.desktop)
target_link_libraries(krunner_kill
                      KF5::I18n
                      KF5::Completion
                      KF5::ConfigWidgets
                      KF5::AuthCore
                      KF5::Runner
                      KSysGuard::ProcessCore
                      )
add_dependencies(krunner_kill kcm_krunner_kill)

install(TARGETS krunner_kill DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner")
install(TARGETS kcm_krunner_kill DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES plasma-runner-kill_config.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
