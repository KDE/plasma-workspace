project(windowedwidgetsrunner)

add_library(krunner_windowedwidgets MODULE windowedwidgetsrunner.cpp)
target_link_libraries(krunner_windowedwidgets KF5::Plasma)

install(TARGETS krunner_windowedwidgets DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-runner-windowedwidgets.desktop DESTINATION ${SERVICES_INSTALL_DIR})
