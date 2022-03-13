add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_windowedwidgets\")

kcoreaddons_add_plugin(krunner_windowedwidgets SOURCES windowedwidgetsrunner.cpp INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_windowedwidgets KF5::Plasma KF5::I18n KF5::Runner)
