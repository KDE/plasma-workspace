add_definitions(-DTRANSLATION_DOMAIN=\"systemmonitor\")

set (KDED_KSYSGUARD_SRCS
     kdedksysguard.cpp
)

add_library(kded_ksysguard MODULE ${KDED_KSYSGUARD_SRCS})

target_link_libraries(kded_ksysguard
    KF5::ProcessCore
    KF5::ProcessUi
    KF5::DBusAddons
    KF5::I18n
    KF5::XmlGui
    KF5::GlobalAccel
)

install(TARGETS kded_ksysguard DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES ksysguard.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kded)

add_executable(systemmonitor ksystemactivitydialog.cpp main.cpp)

target_link_libraries(systemmonitor
    KF5::ProcessCore
    KF5::ProcessUi
    KF5::DBusAddons
    KF5::ConfigCore
    KF5::I18n
    KF5::XmlGui
    KF5::GlobalAccel
    KF5::WindowSystem
)

install(TARGETS systemmonitor DESTINATION ${KDE_INSTALL_BINDIR})
