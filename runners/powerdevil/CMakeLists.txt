add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_powerdevil\")

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(krunner_powerdevil_SRCS
    PowerDevilRunner.cpp
)

kcoreaddons_add_plugin(krunner_powerdevil SOURCES ${krunner_powerdevil_SRCS} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_powerdevil Qt::DBus KF5::ConfigCore KF5::I18n KF5::Plasma KF5::Runner PW::KWorkspace)
