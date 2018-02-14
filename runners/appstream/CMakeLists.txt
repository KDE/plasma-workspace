add_definitions(-DTRANSLATION_DOMAIN="plasma_runner_appstream")

add_library(appstreamrunner MODULE appstreamrunner.cpp)
target_link_libraries(appstreamrunner PUBLIC KF5::Runner KF5::I18n KF5::Service AppStreamQt)

install(TARGETS appstreamrunner DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES appstreamrunner.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
