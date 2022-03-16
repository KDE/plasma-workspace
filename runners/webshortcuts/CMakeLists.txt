add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_webshortcuts\")

kcoreaddons_add_plugin(krunner_webshortcuts SOURCES webshortcutrunner.cpp webshortcutrunner.h INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_webshortcuts
  Qt::DBus
  KF5::Runner
  KF5::Service
  KF5::KIOWidgets
  KF5::I18n)
