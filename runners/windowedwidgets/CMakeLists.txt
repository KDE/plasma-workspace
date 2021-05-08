add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_windowedwidgets\")

kcoreaddons_add_plugin(krunner_windowedwidgets SOURCES windowedwidgetsrunner.cpp INSTALL_NAMESPACE "kf5/krunner")
kcoreaddons_desktop_to_json(krunner_windowedwidgets plasma-runner-windowedwidgets.desktop)
target_link_libraries(krunner_windowedwidgets KF5::Plasma KF5::I18n KF5::Runner)
