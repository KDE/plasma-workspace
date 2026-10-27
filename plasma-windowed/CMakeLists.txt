add_definitions(-DTRANSLATION_DOMAIN=\"plasmawindowed\")

# Core corona+view logic, shared by the plasmawindowed executable and by tests
# that host an applet's AppletQuickItem in their own scene (see autotests/).
# PIC is required because tests link this into a shared QML plugin.
add_library(plasmawindowedcore STATIC
    plasmawindowedcorona.cpp
    plasmawindowedcorona.h
    plasmawindowedview.cpp
    plasmawindowedview.h
)
set_target_properties(plasmawindowedcore PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(plasmawindowedcore PUBLIC
    Qt::Widgets
    Qt::Quick
    Qt::Qml
    KF6::I18n
    KF6::IconThemes
    KF6::XmlGui
    Plasma::PlasmaQuick
    Plasma::Plasma
    KF6::DBusAddons
    KF6::Notifications
    KF6::Package
    KF6::StatusNotifierItem
)

add_executable(plasmawindowed main.cpp)
target_link_libraries(plasmawindowed PRIVATE plasmawindowedcore)

install(TARGETS plasmawindowed ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
#even if hidden, the desktop file is needed anyways for kdbusservice::unique
configure_file(org.kde.plasmawindowed.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmawindowed.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmawindowed.desktop DESTINATION ${KDE_INSTALL_APPDIR})

