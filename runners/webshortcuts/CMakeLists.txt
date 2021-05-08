add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_webshortcuts\")

kcoreaddons_add_plugin(krunner_webshortcuts SOURCES webshortcutrunner.cpp INSTALL_NAMESPACE "kf5/krunner")
kcoreaddons_desktop_to_json(krunner_webshortcuts plasma-runner-webshortcuts.desktop)
target_link_libraries(krunner_webshortcuts
  Qt::DBus
  KF5::Runner
  KF5::KIOWidgets
  KF5::I18n)

install(FILES plasma-runner-webshortcuts_config.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
