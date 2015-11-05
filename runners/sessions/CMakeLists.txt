add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_sessions\")

set(krunner_sessions_SRCS
    sessionrunner.cpp
)

qt5_add_dbus_interface(krunner_sessions_SRCS ${SCREENSAVER_DBUS_INTERFACE} screensaver_interface)

add_library(krunner_sessions MODULE ${krunner_sessions_SRCS})
target_link_libraries(krunner_sessions Qt5::Widgets Qt5::DBus KF5::Runner KF5::I18n PW::KWorkspace)

install(TARGETS krunner_sessions DESTINATION ${KDE_INSTALL_PLUGINDIR} )

install(FILES plasma-runner-sessions.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

