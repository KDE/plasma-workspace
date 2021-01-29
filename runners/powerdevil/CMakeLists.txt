add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_powerdevil\")

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(krunner_powerdevil_SRCS
    PowerDevilRunner.cpp
)

add_library(krunner_powerdevil MODULE ${krunner_powerdevil_SRCS})
kcoreaddons_desktop_to_json(krunner_powerdevil plasma-runner-powerdevil.desktop)
target_link_libraries(krunner_powerdevil Qt::DBus KF5::ConfigCore KF5::I18n KF5::Plasma KF5::Runner PW::KWorkspace)

install(TARGETS krunner_powerdevil DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner")
