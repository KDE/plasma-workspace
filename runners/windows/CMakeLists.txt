add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_windows\")

set(krunner_windows_SRCS
    windowsrunner.cpp
)

add_library(krunner_windows MODULE ${krunner_windows_SRCS})
kcoreaddons_desktop_to_json(krunner_windows plasma-runner-windows.desktop)
target_link_libraries(krunner_windows Qt5::Widgets KF5::WindowSystem KF5::I18n KF5::Runner)

configure_file(config-windowsrunner.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-windowsrunner.h)

if(HAVE_X11)
  target_link_libraries(krunner_windows ${X11_LIBRARIES} XCB::XCB )
  target_link_libraries(krunner_windows Qt5::X11Extras)
endif()

install(TARGETS krunner_windows DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner" )
