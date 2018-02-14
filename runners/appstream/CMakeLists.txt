add_definitions(-DTRANSLATION_DOMAIN="plasma_runner_appstream")

add_library(krunner_appstream MODULE appstreamrunner.cpp)
target_link_libraries(krunner_appstream PUBLIC KF5::Runner KF5::I18n KF5::Service AppStreamQt)

install(TARGETS krunner_appstream DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES plasma-runner-appstream.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
