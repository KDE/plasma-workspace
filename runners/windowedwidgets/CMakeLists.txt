add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_windowedwidgets\")

add_library(krunner_windowedwidgets MODULE windowedwidgetsrunner.cpp)
target_link_libraries(krunner_windowedwidgets KF5::Plasma KF5::I18n KF5::Runner)

install(TARGETS krunner_windowedwidgets DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-runner-windowedwidgets.desktop DESTINATION ${SERVICES_INSTALL_DIR})
