
add_library(holidayeventshelperplugin SHARED holidayeventshelperplugin.cpp)

target_link_libraries(holidayeventshelperplugin Qt5::Qml
                                                Qt5::Core
                                                KF5::ConfigCore
)

install(TARGETS holidayeventshelperplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/holidayeventshelperplugin)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/holidayeventshelperplugin)
