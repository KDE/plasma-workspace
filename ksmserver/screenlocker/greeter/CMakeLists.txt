remove_definitions(-DTRANSLATION_DOMAIN=\"kscreenlocker\")
add_definitions(-DTRANSLATION_DOMAIN=\"kscreenlocker_greet\")

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${KCheckPass_SOURCE_DIR}/kcheckpass
    ../kcheckpass
    ${CMAKE_CURRENT_BINARY_DIR}/../../
)

set(kscreenlocker_greet_SRCS
   authenticator.cpp
   sessions.cpp
   greeterapp.cpp
   main.cpp )

qt5_add_resources(kscreenlocker_greet_SRCS fallbacktheme.qrc)
kconfig_add_kcfg_files(kscreenlocker_greet_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../kcfg/kscreensaversettings.kcfgc)

add_executable(kscreenlocker_greet ${kscreenlocker_greet_SRCS})

target_link_libraries(kscreenlocker_greet
                        PW::KWorkspace
                        KF5::Solid
                        KF5::Plasma
                        KF5::PlasmaQuick
                        KF5::Service
                        KF5::Crash
                        KF5::I18n
                        KF5::ConfigGui
                        KF5::WindowSystem
                        KF5::Solid
                        Qt5::Quick
                        Qt5::Qml
                        Qt5::X11Extras
                        ${X11_LIBRARIES}
                        KF5::KDELibs4Support
                        )

install(TARGETS kscreenlocker_greet DESTINATION ${LIBEXEC_INSTALL_DIR})

install(DIRECTORY themes/org.kde.passworddialog DESTINATION ${DATA_INSTALL_DIR}/ksmserver/screenlocker)

add_subdirectory(autotests)
