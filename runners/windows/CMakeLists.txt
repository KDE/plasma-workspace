set(krunner_windows_SRCS
    windowsrunner.cpp
)

add_library(krunner_windows MODULE ${krunner_windows_SRCS})
target_link_libraries(krunner_windows ${KDE4_KDEUI_LIBS} KF5::Plasma)

install(TARGETS krunner_windows DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES plasma-runner-windows.desktop DESTINATION ${SERVICES_INSTALL_DIR})
