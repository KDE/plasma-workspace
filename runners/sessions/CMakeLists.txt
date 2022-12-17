add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_sessions\")

set(krunner_sessions_SRCS
    sessionrunner.cpp
)

qt_add_dbus_interface(krunner_sessions_SRCS ${SCREENSAVER_DBUS_INTERFACE} screensaver_interface)

kcoreaddons_add_plugin(krunner_sessions SOURCES ${krunner_sessions_SRCS} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_sessions Qt::Widgets Qt::DBus KF5::Runner KF5::I18n KF5::ConfigCore KF5::WidgetsAddons PW::KWorkspace)
