add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_webshortcuts\")

set(krunner_webshortcuts_SRCS
    webshortcutrunner.cpp
)

add_library(krunner_webshortcuts MODULE ${krunner_webshortcuts_SRCS})
kcoreaddons_desktop_to_json(krunner_webshortcuts plasma-runner-webshortcuts.desktop)
target_link_libraries(krunner_webshortcuts
  Qt::DBus
  KF5::Runner
  KF5::KIOWidgets
  KF5::I18n)

set_target_properties(krunner_webshortcuts PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/krunner")
install(TARGETS krunner_webshortcuts DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner" )

install(FILES plasma-runner-webshortcuts_config.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
