set (KDED_KSYSGUARD_SRCS
     kdedksysguard.cpp
)

add_library(kded_ksysguard MODULE ${KDED_KSYSGUARD_SRCS})

target_link_libraries(kded_ksysguard
    KF5::ProcessCore
    KF5::ProcessUi
    KF5::DBusAddons
    KF5::XmlGui
    KF5::GlobalAccel
    KF5::KDELibs4Support
)

install(TARGETS kded_ksysguard DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kded_ksysguard.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)

add_executable(systemmonitor ksystemactivitydialog.cpp main.cpp)

target_link_libraries(systemmonitor
    KF5::ProcessCore
    KF5::ProcessUi
    KF5::DBusAddons
    KF5::XmlGui
    KF5::GlobalAccel
    KF5::KDELibs4Support
)

install(TARGETS systemmonitor DESTINATION ${BIN_INSTALL_DIR})
