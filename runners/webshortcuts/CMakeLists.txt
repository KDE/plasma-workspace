add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_webshortcuts\")

set(krunner_webshortcuts_SRCS
    webshortcutrunner.cpp
)

add_library(krunner_webshortcuts MODULE ${krunner_webshortcuts_SRCS})
target_link_libraries(krunner_webshortcuts
  Qt5::DBus
  KF5::Runner
  KF5::KIOWidgets
  KF5::I18n)

install(TARGETS krunner_webshortcuts DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES plasma-runner-webshortcuts.desktop DESTINATION ${SERVICES_INSTALL_DIR})
