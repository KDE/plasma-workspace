add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_powerdevil\")

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(krunner_powerdevil_SRCS
    PowerDevilRunner.cpp
)

kcoreaddons_add_plugin(krunner_powerdevil SOURCES ${krunner_powerdevil_SRCS} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_powerdevil Qt::DBus KF6::ConfigCore KF6::I18n KF6::Plasma KF6::Runner PW::KWorkspace)
