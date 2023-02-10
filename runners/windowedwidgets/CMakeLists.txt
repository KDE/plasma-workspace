add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_windowedwidgets\")

kcoreaddons_add_plugin(krunner_windowedwidgets SOURCES windowedwidgetsrunner.cpp INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_windowedwidgets KF6::Plasma KF6::I18n KF6::Runner)
