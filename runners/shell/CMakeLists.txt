set(krunner_shell_SRCS
    shellrunner.cpp
    shell_config.cpp
)

qt5_wrap_ui(krunner_shell_SRCS shellOptions.ui)
add_library(krunner_shell MODULE ${krunner_shell_SRCS})
target_link_libraries(krunner_shell
    KF5::KIOCore
    KF5::I18n
    KF5::Plasma
    KF5::Runner
    KF5::Su
    KF5::KDE4Support
)

install(TARGETS krunner_shell DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-runner-shell.desktop DESTINATION ${SERVICES_INSTALL_DIR})

