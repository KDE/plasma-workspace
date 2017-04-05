add_library(appstreamrunner MODULE appstreamrunner.cpp)
target_link_libraries(appstreamrunner PUBLIC KF5::Runner KF5::I18n KF5::Service AppStreamQt)

install(TARGETS appstreamrunner DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES appstreamrunner.desktop DESTINATION ${SERVICES_INSTALL_DIR})
