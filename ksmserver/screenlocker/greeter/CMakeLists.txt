include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${KCheckPass_SOURCE_DIR}/kcheckpass
)

set(kscreenlocker_greet_SRCS
   sessions.cpp
   greeterapp.cpp
   screensaverwindow.cpp
   main.cpp )

kconfig_add_kcfg_files(kscreenlocker_greet_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../kcfg/kscreensaversettings.kcfgc)

add_executable(kscreenlocker_greet ${kscreenlocker_greet_SRCS})

target_link_libraries(kscreenlocker_greet
                        PW::KWorkspace
                        KF5::Solid
                        KF5::Plasma
                        KF5::Service
                        KF5::Declarative
                        KF5::Crash
                        Qt5::Quick
                        Qt5::Qml
                        Qt5::X11Extras
                        ${X11_LIBRARIES}
                        KF5::KDE4Support
                        )

install(TARGETS kscreenlocker_greet DESTINATION ${LIBEXEC_INSTALL_DIR})

install(DIRECTORY themes/org.kde.passworddialog DESTINATION ${DATA_INSTALL_DIR}/ksmserver/screenlocker)
