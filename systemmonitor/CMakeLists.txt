add_definitions(-DTRANSLATION_DOMAIN=\"systemmonitor\")

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")

kcoreaddons_add_plugin(ksysguard SOURCES kdedksysguard.cpp kdedksysguard.h INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kded")

target_link_libraries(ksysguard
    KF6::CoreAddons
    KF6::DBusAddons
    KF6::I18n
    KF6::XmlGui
    KF6::GlobalAccel
)

add_executable(systemmonitor ksystemactivitydialog.cpp ksystemactivitydialog.h main.cpp)

target_link_libraries(systemmonitor
    KSysGuard::ProcessUi
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::I18n
    KF6::XmlGui
    KF6::GlobalAccel
    KF6::WindowSystem
    PW::KWorkspace
)

install(TARGETS systemmonitor DESTINATION ${KDE_INSTALL_BINDIR})
install(PROGRAMS org.kde.systemmonitor.desktop DESTINATION ${KDE_INSTALL_APPDIR})
