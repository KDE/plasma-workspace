set(notificationshelper_SRCS
    notificationshelper.cpp
    notificationshelperplugin.cpp
    )

add_library(notificationshelperplugin SHARED ${notificationshelper_SRCS})
target_link_libraries(notificationshelperplugin Qt5::Core
                                                Qt5::Gui
                                                Qt5::Qml
                                                Qt5::Quick)

install(TARGETS notificationshelperplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/notifications)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/notifications)
