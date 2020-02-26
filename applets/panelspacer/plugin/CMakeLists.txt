kde_enable_exceptions()

add_definitions(-DTRANSLATION_DOMAIN=\"panelspacer\")

set(panelspacer_SRCS
    panelspacer.cpp)

add_library(org.kde.plasma.panelspacer MODULE ${panelspacer_SRCS})

kcoreaddons_desktop_to_json(org.kde.plasma.panelspacer ../package/metadata.desktop)

target_link_libraries(org.kde.plasma.panelspacer Qt5::Gui Qt5::Core Qt5::Qml Qt5::Quick KF5::Plasma KF5::PlasmaQuick KF5::I18n)

install(TARGETS org.kde.plasma.panelspacer DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)

