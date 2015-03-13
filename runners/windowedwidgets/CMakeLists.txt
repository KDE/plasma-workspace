add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_windowedwidgets\")

add_library(krunner_windowedwidgets MODULE windowedwidgetsrunner.cpp)
target_link_libraries(krunner_windowedwidgets KF5::Plasma KF5::I18n KF5::Runner)

install(TARGETS krunner_windowedwidgets DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES plasma-runner-windowedwidgets.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
