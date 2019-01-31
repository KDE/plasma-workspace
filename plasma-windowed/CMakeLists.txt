add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

set(plasmawindowed-app_SRCS
    plasmawindowedcorona.cpp
    plasmawindowedview.cpp
    main.cpp
)

add_executable(plasmawindowed ${plasmawindowed-app_SRCS})

target_link_libraries(
    plasmawindowed
    Qt5::Widgets
    Qt5::Quick
    Qt5::Qml
    KF5::I18n
    KF5::IconThemes
    KF5::XmlGui
    KF5::QuickAddons
    KF5::PlasmaQuick
    KF5::Plasma
    KF5::DBusAddons
    KF5::Notifications
)

install(TARGETS plasmawindowed ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
#even if hidden, the desktop file is needed anyways for kdbusservice::unique
install(FILES plasma-windowed.desktop DESTINATION ${KDE_INSTALL_APPDIR})
