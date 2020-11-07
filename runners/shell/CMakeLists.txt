add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_shell\")

set(krunner_shell_SRCS shellrunner.cpp)

add_library(krunner_shell MODULE ${krunner_shell_SRCS})
kcoreaddons_desktop_to_json(krunner_shell plasma-runner-shell.desktop)
target_link_libraries(krunner_shell
    KF5::KIOCore
    KF5::KIOGui
    KF5::I18n
    KF5::Notifications
    KF5::Plasma
    KF5::Runner
    KF5::Completion
)

install(TARGETS krunner_shell DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner")

if(BUILD_TESTING)
add_subdirectory(autotests)
endif()

