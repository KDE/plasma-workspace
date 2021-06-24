# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"krdb\")

add_library(krdb krdb.cpp)
target_link_libraries(krdb PRIVATE Qt::Widgets Qt::DBus Qt5::X11Extras KF5::GuiAddons KF5::I18n KF5::WindowSystem KF5::ConfigWidgets PW::KWorkspace X11::X11)

install(TARGETS krdb ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
