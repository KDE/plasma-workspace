set(krunner_sessions_SRCS
    sessionrunner.cpp
)

set(screensaver_xml ${ksmserver_SOURCE_DIR}/screenlocker/dbus/org.freedesktop.ScreenSaver.xml)
qt5_add_dbus_interface(krunner_sessions_SRCS ${screensaver_xml} screensaver_interface)

add_library(krunner_sessions MODULE ${krunner_sessions_SRCS})
target_link_libraries(krunner_sessions KIOCore KF5::Plasma PW::KWorkspace)

install(TARGETS krunner_sessions DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES plasma-runner-sessions.desktop DESTINATION ${SERVICES_INSTALL_DIR})

