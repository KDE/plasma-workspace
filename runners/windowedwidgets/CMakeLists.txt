add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_windowedwidgets\")

add_library(krunner_windowedwidgets MODULE windowedwidgetsrunner.cpp)
kcoreaddons_desktop_to_json(krunner_windowedwidgets plasma-runner-windowedwidgets.desktop)
target_link_libraries(krunner_windowedwidgets KF5::Plasma KF5::I18n KF5::Runner)

install(TARGETS krunner_windowedwidgets DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner")
